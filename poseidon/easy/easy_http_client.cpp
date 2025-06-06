// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_http_client.hpp"
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
        shptr<HTTP_Client_Session> session;
        shptr<DNS_Connect_Task> dns_task;
        cacheline_barrier xcb_1;

        // shared fields between threads
        struct Event
          {
            Easy_HTTP_Event type;
            HTTP_Response_Headers resp;
            linear_buffer data;
            bool close_now = false;
          };

        ::std::deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    ::std::unordered_map<const volatile HTTP_Client_Session*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HTTP_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    const volatile HTTP_Client_Session* m_refptr;

    Final_Fiber(const Easy_HTTP_Client::callback_type& callback,
                const shptr<Session_Table>& sessions,
                const volatile HTTP_Client_Session* refptr)
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

          if(ROCKET_UNEXPECT(event.type == easy_http_close)) {
            // This will be the last event on this session.
            queue = nullptr;
            sessions->session_map.erase(session_iter);
          }
          session_iter = sessions->session_map.end();
          lock.unlock();

          try {
            // Process a response.
            this->m_callback(session, *this, event.type, move(event.resp), move(event.data));
          }
          catch(exception& stdex) {
            // Shut the connection down asynchronously. Pending output data
            // are discarded, but the user-defined callback will still be called
            // for remaining input data, in case there is something useful.
            POSEIDON_LOG_ERROR(("Unhandled exception thrown from HTTP client: $1"), stdex);
            session->quick_close();
          }

          if(event.close_now)
            session->tcp_shut_down();
        }
      }
  };

struct Final_Session final : HTTP_Client_Session
  {
    Easy_HTTP_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Session(const Easy_HTTP_Client::callback_type& callback,
                  const shptr<Session_Table>& sessions)
      :
        TCP_Socket(), HTTP_Client_Session(),
        m_callback(callback), m_wsessions(sessions)
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
          this->quick_close();
        }
      }

    virtual
    void
    do_on_tcp_connected() override
      {
        Session_Table::Event_Queue::Event event;
        event.type = easy_http_open;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_http_response_finish(HTTP_Response_Headers&& resp, linear_buffer&& data,
                               bool close_now) override
      {
        Session_Table::Event_Queue::Event event;
        event.type = easy_http_message;
        event.resp = move(resp);
        event.data = move(data);
        event.close_now = close_now;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_abstract_socket_on_closed() override
      {
        char sbuf[1024];
        int err_code = errno;
        const char* err_str = ::strerror_r(err_code, sbuf, sizeof(sbuf));

        Session_Table::Event_Queue::Event event;
        event.type = easy_http_close;
        event.data.puts(err_str);
        this->do_push_event_common(move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_HTTP_Client,
  Session_Table);

Easy_HTTP_Client::
~Easy_HTTP_Client()
  {
  }

shptr<HTTP_Client_Session>
Easy_HTTP_Client::
connect(chars_view addr)
  {
    // Parse the address string, which shall contain a host name and an optional
    // port to connect, followed by an optional path and an optional query string.
    // If no port is specified, 80 is implied.
    Network_Reference caddr;
    if(parse_network_reference(caddr, addr) != addr.n)
      POSEIDON_THROW(("Invalid connect address `$1`"), addr);

    if(caddr.port.n == 0)
      caddr.port_num = 80;

    // Disallow superfluous components.
    if(caddr.path.p != nullptr)
      POSEIDON_THROW(("URI path shall not be specified in address `$1`"), addr);

    if(caddr.query.p != nullptr)
      POSEIDON_THROW(("URI query shall not be specified in address `$1`"), addr);

    if(caddr.fragment.p != nullptr)
      POSEIDON_THROW(("URI fragment shall not be specified in address `$1`"), addr);

    // Pre-allocate necessary objects. The entire operation will be atomic.
    if(!this->m_sessions)
      this->m_sessions = new_sh<X_Session_Table>();

    auto session = new_sh<Final_Session>(this->m_callback, this->m_sessions);
    session->http_set_default_host(format_string("$1:$2", caddr.host, caddr.port_num));
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
Easy_HTTP_Client::
close_all() noexcept
  {
    this->m_sessions = nullptr;
  }

}  // namespace poseidon
