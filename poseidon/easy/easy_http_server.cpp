// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_http_server.hpp"
#include "../socket/tcp_acceptor.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Session_Table
  {
    struct Event_Queue
      {
        // read-only fields; no locking needed
        shptr<HTTP_Server_Session> session;
        cacheline_barrier xcb_1;

        // shared fields between threads
        struct Event
          {
            Easy_HTTP_Event type;
            HTTP_Request_Headers req;
            linear_buffer data;
            bool close_now = false;
            HTTP_Status status = http_status_null;
          };

        ::std::deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    ::std::unordered_map<const volatile HTTP_Server_Session*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HTTP_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    const volatile HTTP_Server_Session* m_refptr;

    Final_Fiber(const Easy_HTTP_Server::callback_type& callback,
                const shptr<Session_Table>& sessions,
                const volatile HTTP_Server_Session* refptr)
      :
        m_callback(callback), m_wsessions(sessions), m_refptr(refptr)
      { }

    virtual
    void
    do_on_abstract_fiber_execute() override
      {
        for(;;) {
          // The event callback may stop this server, so we have to check for
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
            if(event.status != http_status_null) {
              // Send a bad request response.
              HTTP_Response_Headers resp;
              resp.status = event.status;
              resp.headers.emplace_back(&"Connection", &"close");
              session->http_response(move(resp), "");
            }
            else {
              // Process a request.
              this->m_callback(session, *this, event.type, move(event.req), move(event.data));
            }

            if(event.close_now)
              session->tcp_shut_down();
          }
          catch(exception& stdex) {
            // Shut the connection down with a message.
            // XXX: The user-defined callback may have sent a response...?
            POSEIDON_LOG_ERROR(("Unhandled exception thrown from HTTP client: $1"), stdex);
            HTTP_Response_Headers resp;
            resp.status = http_status_internal_server_error;
            resp.headers.emplace_back(&"Connection", &"close");
            session->http_response(move(resp), "");
            session->tcp_shut_down();
          }
        }
      }
  };

struct Final_Session final : HTTP_Server_Session
  {
    Easy_HTTP_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Session(const Easy_HTTP_Server::callback_type& callback,
                  unique_posix_fd&& fd,
                  const shptr<Session_Table>& sessions)
      :
        TCP_Socket(move(fd)),
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
    do_on_http_request_finish(HTTP_Request_Headers&& req, linear_buffer&& data,
                              bool close_now) override
      {
        Session_Table::Event_Queue::Event event;
        event.type = easy_http_message;
        event.req = move(req);
        event.data = move(data);
        event.close_now = close_now;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_http_request_error(HTTP_Status status) override
      {
        Session_Table::Event_Queue::Event event;
        event.status = status;
        event.close_now = true;
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

struct Final_Acceptor final : TCP_Acceptor
  {
    Easy_HTTP_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Acceptor(const Easy_HTTP_Server::callback_type& callback,
                   const IPv6_Address& addr, const shptr<Session_Table>& sessions)
      :
        TCP_Acceptor(addr),
        m_callback(callback), m_wsessions(sessions)
      {
        this->defer_accept(10s);
      }

    virtual
    shptr<Abstract_Socket>
    do_accept_socket_opt(const IPv6_Address& addr, unique_posix_fd&& fd) override
      {
        auto sessions = this->m_wsessions.lock();
        if(!sessions)
          return nullptr;

        auto session = new_sh<Final_Session>(this->m_callback, move(fd), sessions);
        (void) addr;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(sessions->mutex);

        auto r = sessions->session_map.try_emplace(session.get());
        ROCKET_ASSERT(r.second);
        r.first->second.session = session;
        return session;
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_HTTP_Server,
  Session_Table);

Easy_HTTP_Server::
~Easy_HTTP_Server()
  {
  }

shptr<TCP_Acceptor>
Easy_HTTP_Server::
start(const IPv6_Address& addr)
  {
    auto sessions = new_sh<X_Session_Table>();
    auto acceptor = new_sh<Final_Acceptor>(this->m_callback, addr, sessions);

    network_driver.insert(acceptor);
    this->m_sessions = move(sessions);
    this->m_acceptor = acceptor;
    return acceptor;
  }

shptr<TCP_Acceptor>
Easy_HTTP_Server::
start(const cow_string& addr)
  {
    IPv6_Address v6addr(addr);
    return this->start(v6addr);
  }

shptr<TCP_Acceptor>
Easy_HTTP_Server::
start_any(uint16_t port)
  {
    IPv6_Address v6addr(ipv6_unspecified, port);
    return this->start(v6addr);
  }

void
Easy_HTTP_Server::
stop() noexcept
  {
    this->m_sessions = nullptr;
    this->m_acceptor = nullptr;
  }

}  // namespace poseidon
