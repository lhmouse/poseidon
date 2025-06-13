// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_wss_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../socket/dns_connect_task.hpp"
#include "../static/task_executor.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Session_Table
  {
    struct Event_Queue
      {
        // read-only fields; no locking needed
        shptr<WSS_Client_Session> session;
        shptr<DNS_Connect_Task> dns_task;
        cacheline_barrier xcb_1;

        // shared fields between threads
        struct Event
          {
            Easy_WS_Event type;
            linear_buffer data;
          };

        ::std::deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    ::std::unordered_map<volatile WSS_Client_Session*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_WSS_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    volatile WSS_Client_Session* m_refptr;

    Final_Fiber(const Easy_WSS_Client::callback_type& callback,
                const shptr<Session_Table>& sessions, volatile WSS_Client_Session* refptr)
      :
        m_callback(callback), m_wsessions(sessions), m_refptr(refptr)
      { }

    virtual
    void
    do_on_abstract_fiber_execute() override
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
            session->wss_shut_down(websocket_status_unexpected_error);
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

struct Final_Session final : WSS_Client_Session
  {
    Easy_WSS_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    uniptr<Request_URI> m_uri;

    Final_Session(const Easy_WSS_Client::callback_type& callback,
                  const shptr<Session_Table>& sessions, uniptr<Request_URI>&& uri)
      :
        SSL_Socket(network_driver), HTTPS_Client_Session(uri->host),
        WSS_Client_Session(uri->path, uri->query),
        m_callback(callback), m_wsessions(sessions), m_uri(move(uri))
      { }

    void
    do_push_event_common(Session_Table::Event_Queue::Event&& event)
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
          this->close();
        }
      }

    virtual
    void
    do_on_wss_connected() override
      {
        Session_Table::Event_Queue::Event event;
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
    do_on_wss_message_finish(WebSocket_Opcode opcode, linear_buffer&& data) override
      {
        Session_Table::Event_Queue::Event event;
        if(opcode == websocket_TEXT)
          event.type = easy_ws_text;
        else if(opcode == websocket_BINARY)
          event.type = easy_ws_binary;
        else if(opcode == websocket_PONG)
          event.type = easy_ws_pong;
        else
          return;

        event.data.swap(data);
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_wss_close(WebSocket_Status status, chars_view reason) override
      {
        Session_Table::Event_Queue::Event event;
        event.type = easy_ws_close;

        tinyfmt_ln fmt;
        fmt << status << ": " << reason;
        event.data = fmt.extract_buffer();

        this->do_push_event_common(move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_WSS_Client,
  Session_Table);

Easy_WSS_Client::
~Easy_WSS_Client()
  {
  }

shptr<WSS_Client_Session>
Easy_WSS_Client::
connect(const cow_string& addr, const callback_type& callback)
  {
    // Parse the address string, which shall contain a host name and an optional
    // port to connect, followed by an optional path and an optional query string.
    // If no port is specified, 443 is implied.
    Network_Reference caddr;
    if(parse_network_reference(caddr, addr) != addr.size())
      POSEIDON_THROW(("Invalid address `$1`"), addr);

    if(caddr.port.n == 0)
      caddr.port_num = 443;

    auto uri = new_uni<Request_URI>();
    uri->host = sformat("$1:$2", caddr.host, caddr.port_num);
    uri->path = cow_string(caddr.path);
    uri->query = cow_string(caddr.query);
    uri->fragment = cow_string(caddr.fragment);

    // Pre-allocate necessary objects. The entire operation will be atomic.
    if(!this->m_sessions)
      this->m_sessions = new_sh<X_Session_Table>();

    auto session = new_sh<Final_Session>(callback, this->m_sessions, move(uri));
    auto dns_task = new_sh<DNS_Connect_Task>(network_driver,
                       session, cow_string(caddr.host), caddr.port_num);

    // Initiate the connection.
    plain_mutex::unique_lock lock(this->m_sessions->mutex);

    task_executor.enqueue(dns_task);
    auto r = this->m_sessions->session_map.try_emplace(session.get());
    ROCKET_ASSERT(r.second);
    r.first->second.session = session;
    r.first->second.dns_task = move(dns_task);
    return session;
  }

void
Easy_WSS_Client::
close_all() noexcept
  {
    this->m_sessions = nullptr;
  }

}  // namespace poseidon
