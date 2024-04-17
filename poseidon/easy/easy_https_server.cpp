// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_https_server.hpp"
#include "../socket/listen_socket.hpp"
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
        shptr<HTTPS_Server_Session> session;
        cacheline_barrier xcb_1;

        // shared fields between threads
        struct Event
          {
            Easy_HTTP_Event type;
            HTTP_Request_Headers req;
            linear_buffer data;
            bool close_now = false;
            uint32_t status = 0;
          };

        deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    unordered_map<const volatile HTTPS_Server_Session*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HTTPS_Server::thunk_type m_thunk;
    wkptr<Session_Table> m_wsessions;
    const volatile HTTPS_Server_Session* m_refptr;

    Final_Fiber(const Easy_HTTPS_Server::thunk_type& thunk, shptrR<Session_Table> sessions,
                const volatile HTTPS_Server_Session* refptr)
      :
        m_thunk(thunk), m_wsessions(sessions), m_refptr(refptr)
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
            if(event.status != 0) {
              // Send a bad request response.
              HTTP_Response_Headers resp;
              resp.status = event.status;
              resp.headers.emplace_back(&"Connection", &"close");
              session->https_response(move(resp), "");
            }
            else {
              // Process a request.
              this->m_thunk(session, *this, event.type, move(event.req), move(event.data));
            }

            if(event.close_now)
              session->ssl_shut_down();
          }
          catch(exception& stdex) {
            // Shut the connection down with a message.
            // XXX: The user-defined callback may have sent a response...?
            POSEIDON_LOG_ERROR(("Unhandled exception thrown from1"), stdex);
            HTTP_Response_Headers resp;
            resp.status = 500;
            resp.headers.emplace_back(&"Connection", &"close");
            session->https_response(move(resp), "");
            session->ssl_shut_down();
          }
        }
      }
  };

struct Final_Session final : HTTPS_Server_Session
  {
    Easy_HTTPS_Server::thunk_type m_thunk;
    wkptr<Session_Table> m_wsessions;

    Final_Session(const Easy_HTTPS_Server::thunk_type& thunk, unique_posix_fd&& fd,
                  shptrR<Session_Table> sessions)
      :
        SSL_Socket(move(fd), network_driver), m_thunk(thunk), m_wsessions(sessions)
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
            fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_thunk, sessions, this));
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
    do_on_ssl_connected() override
      {
        Session_Table::Event_Queue::Event event;
        event.type = easy_http_open;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_https_request_finish(HTTP_Request_Headers&& req, linear_buffer&& data,
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
    do_on_https_request_error(uint32_t status) override
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

struct Final_Listener final : Listen_Socket
  {
    Easy_HTTPS_Server::thunk_type m_thunk;
    wkptr<Session_Table> m_wsessions;

    Final_Listener(const Easy_HTTPS_Server::thunk_type& thunk, const IPv6_Address& addr,
                   shptrR<Session_Table> sessions)
      :
        Listen_Socket(addr), m_thunk(thunk), m_wsessions(sessions)
      {
        this->defer_accept(10s);
      }

    virtual
    shptr<Abstract_Socket>
    do_on_listen_new_client_opt(IPv6_Address&& addr, unique_posix_fd&& fd) override
      {
        auto sessions = this->m_wsessions.lock();
        if(!sessions)
          return nullptr;

        auto session = new_sh<Final_Session>(this->m_thunk, move(fd), sessions);
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

POSEIDON_HIDDEN_X_STRUCT(Easy_HTTPS_Server,
  Session_Table);

Easy_HTTPS_Server::
~Easy_HTTPS_Server()
  {
  }

void
Easy_HTTPS_Server::
start(chars_view addr)
  {
    IPv6_Address saddr(addr);
    auto sessions = new_sh<X_Session_Table>();
    auto listener = new_sh<Final_Listener>(this->m_thunk, saddr, sessions);

    network_driver.insert(listener);
    this->m_sessions = move(sessions);
    this->m_listener = move(listener);
  }

void
Easy_HTTPS_Server::
stop() noexcept
  {
    this->m_sessions = nullptr;
    this->m_listener = nullptr;
  }

const IPv6_Address&
Easy_HTTPS_Server::
local_address() const noexcept
  {
    if(!this->m_listener)
      return ipv6_invalid;

    return this->m_listener->local_address();
  }

}  // namespace poseidon
