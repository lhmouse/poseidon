// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_ws_client.hpp"
#include "../static/network_scheduler.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../socket/dns_connect_task.hpp"
#include "../static/task_scheduler.hpp"
#include "../utils.hpp"
#include <deque>
#include <unordered_map>
namespace poseidon {
namespace {

struct Event
  {
    Easy_WS_Event type;
    linear_buffer data;
  };

struct Event_Queue
  {
    // read-only fields; no locking needed
    shptr<WS_Client_Session> session;
    cacheline_barrier xcb_1;

    // shared fields between threads
    ::std::deque<Event> events;
    bool fiber_active = false;
  };

struct Session_Table
  {
    mutable plain_mutex mutex;
    ::std::unordered_map<volatile WS_Client_Session*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_WS_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    volatile WS_Client_Session* m_refptr;

    Final_Fiber(const Easy_WS_Client::callback_type& callback,
                const shptr<Session_Table>& sessions,
                volatile WS_Client_Session* refptr)
      :
        m_callback(callback), m_wsessions(sessions), m_refptr(refptr)
      { }

    virtual
    void
    do_on_abstract_fiber_execute()
      override
      {
        for(;;) {
          // The event callback may stop this client, so we have to check for
          // expiry in every iteration.
          auto sessions = this->m_wsessions.lock();
          if(!sessions)
            return;

          // Pop an event and invoke the user-defined callback here in the
          // main thread. Exceptions are ignored.
          plain_mutex::unique_lock lock(sessions->mutex);

          auto session_iter = sessions->session_map.find(this->m_refptr);
          if(session_iter == sessions->session_map.end())
            return;

          if(session_iter->second.events.empty()) {
            // Terminate now.
            session_iter->second.fiber_active = false;
            return;
          }

          // After `sessions->mutex` is unlocked, other threads may modify
          // `sessions->session_map` and invalidate all iterators, so maintain a
          // reference outside it for safety.
          auto queue = &(session_iter->second);
          ROCKET_ASSERT(queue->fiber_active);
          auto session = queue->session;
          auto event = move(queue->events.front());
          queue->events.pop_front();

          if(ROCKET_UNEXPECT(event.type == easy_ws_close)) {
            // This will be the last event on this session.
            queue = nullptr;
            sessions->session_map.erase(session_iter);
          }
          session_iter = sessions->session_map.end();
          lock.unlock();

          try {
            // Process a message.
            this->m_callback(session, *this, event.type, move(event.data));
          }
          catch(exception& stdex) {
            // Shut the connection down with a message.
            POSEIDON_LOG_ERROR(("Unhandled exception: $1"), stdex);
            session->ws_shut_down(ws_status_unexpected_error);
          }
        }
      }
  };

struct Request_URI
  {
    cow_string host;
    cow_string path;
    cow_string query;
    cow_string fragment;
  };

struct Final_Session final : WS_Client_Session
  {
    Easy_WS_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    uniptr<Request_URI> m_uri;

    Final_Session(const Easy_WS_Client::callback_type& callback,
                  const shptr<Session_Table>& sessions, uniptr<Request_URI>&& uri)
      :
        TCP_Socket(), HTTP_Client_Session(uri->host),
        WS_Client_Session(uri->path, uri->query),
        m_callback(callback), m_wsessions(sessions), m_uri(move(uri))
      { }

    void
    do_push_event_common(Event&& event)
      {
        auto sessions = this->m_wsessions.lock();
        if(!sessions)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(sessions->mutex);

        auto session_iter = sessions->session_map.find(this);
        if(session_iter == sessions->session_map.end())
          return;

        try {
          if(!session_iter->second.fiber_active) {
            // Create a new fiber, if none is active. The fiber shall only reset
            // `m_fiber_private_buffer` if no event is pending.
            fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_callback, sessions, this));
            session_iter->second.fiber_active = true;
          }

          session_iter->second.events.push_back(move(event));
        }
        catch(exception& stdex) {
          POSEIDON_LOG_ERROR(("Could not push network event: $1"), stdex);
          sessions->session_map.erase(session_iter);
          this->quick_shut_down();
        }
      }

    virtual
    void
    do_on_ws_connected()
      override
      {
        Event event;
        event.type = easy_ws_open;

        auto uri = move(this->m_uri);
        ROCKET_ASSERT(uri);

        event.data.putn(uri->host.data(), uri->host.size());
        event.data.putn(uri->path.data(), uri->path.size());
        event.data.putc('?');
        event.data.putn(uri->query.data(), uri->query.size());
        event.data.putc('#');
        event.data.putn(uri->fragment.data(), uri->fragment.size());

        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ws_message_finish(WS_Opcode opcode, linear_buffer&& data)
      override
      {
        Event event;
        if(opcode == ws_TEXT)
          event.type = easy_ws_text;
        else if(opcode == ws_BINARY)
          event.type = easy_ws_binary;
        else if(opcode == ws_PONG)
          event.type = easy_ws_pong;
        else
          return;

        event.data.swap(data);
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ws_close(WS_Status status, chars_view reason)
      override
      {
        Event event;
        event.type = easy_ws_close;

        tinyfmt_ln fmt;
        fmt << status << ": " << reason;
        event.data = fmt.extract_buffer();

        this->do_push_event_common(move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_WS_Client,
  Session_Table);

Easy_WS_Client::
~Easy_WS_Client()
  {
  }

shptr<WS_Client_Session>
Easy_WS_Client::
connect(const cow_string& addr, const callback_type& callback)
  {
    // Parse the address string, which shall contain a host name and an optional
    // port to connect, followed by an optional path and an optional query string.
    // If no port is specified, 80 is implied.
    Network_Reference caddr;
    if(parse_network_reference(caddr, addr) != addr.size())
      POSEIDON_THROW(("Invalid address `$1`"), addr);

    if(caddr.port.n == 0)
      caddr.port_num = 80;

    auto uri = new_uni<Request_URI>();
    uri->host = sformat("$1:$2", caddr.host, caddr.port_num);
    uri->path = cow_string(caddr.path);
    uri->query = cow_string(caddr.query);
    uri->fragment = cow_string(caddr.fragment);

    // Pre-allocate necessary objects. The entire operation will be atomic.
    if(!this->m_sessions)
      this->m_sessions = new_sh<X_Session_Table>();

    auto session = new_sh<Final_Session>(callback, this->m_sessions, move(uri));
    auto dns_task = new_sh<DNS_Connect_Task>(network_scheduler,
                       session, cow_string(caddr.host), caddr.port_num);

    // Initiate the connection.
    plain_mutex::unique_lock lock(this->m_sessions->mutex);

    task_scheduler.launch(dns_task);
    auto r = this->m_sessions->session_map.try_emplace(session.get());
    ROCKET_ASSERT(r.second);
    r.first->second.session = session;
    return session;
  }

void
Easy_WS_Client::
close_all()
  noexcept
  {
    this->m_sessions = nullptr;
  }

}  // namespace poseidon
