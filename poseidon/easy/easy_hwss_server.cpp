// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_hwss_server.hpp"
#include "../socket/tcp_acceptor.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Event
  {
    Easy_HWS_Event type;
    linear_buffer data;
  };

struct Event_Queue
  {
    // read-only fields; no locking needed
    shptr<WSS_Server_Session> session;
    cacheline_barrier xcb_1;

    // shared fields between threads
    ::std::deque<Event> events;
    bool fiber_active = false;
  };

struct Session_Table
  {
    mutable plain_mutex mutex;
    ::std::unordered_map<volatile WSS_Server_Session*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HWSS_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    volatile WSS_Server_Session* m_refptr;

    Final_Fiber(const Easy_HWSS_Server::callback_type& callback,
                const shptr<Session_Table>& sessions,
                volatile WSS_Server_Session* refptr)
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

          if(ROCKET_UNEXPECT(event.type == easy_hws_close)) {
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
            session->wss_shut_down(ws_status_unexpected_error);
          }
        }
      }
  };

struct Final_Session final : WSS_Server_Session
  {
    Easy_HWSS_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Session(unique_posix_fd&& fd,
                  const Easy_HWSS_Server::callback_type& callback,
                  const shptr<Session_Table>& sessions)
      :
        SSL_Socket(move(fd), network_driver), WSS_Server_Session(),
        m_callback(callback), m_wsessions(sessions)
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
          quick_shut_down();
        }
      }

    virtual
    HTTP_Payload_Type
    do_on_https_request_headers(HTTP_C_Headers& req, bool eot) override
      {
        if(req.is_proxy) {
          // Reject proxy requests.
          this->do_on_https_request_error(http_status_forbidden);
          return http_payload_normal;
        }

        Event event;
        switch((uint64_t) req.method)
          {
          case http_OPTIONS:
            // Respond to an XSS check.
            this->do_wss_complete_handshake(req, eot);
            return http_payload_normal;

          case http_GET:
            if(any_of(req.headers, [&](const auto& r) { return r.first == "Upgrade";  })) {
              // Try upgrading to WebSocket.
              this->do_wss_complete_handshake(req, eot);
              return http_payload_normal;
            }

            // Handle a plain HTTP GET request.
            event.type = easy_hws_get;
            break;

          case http_HEAD:
            // Handle a plain HTTP HEAD request.
            event.type = easy_hws_head;
            break;

          default:
            // Reject all the other.
            this->do_on_https_request_error(http_status_method_not_allowed);
            this->quick_shut_down();
            return http_payload_normal;
          }

        event.data.putn(req.raw_host.data(), req.raw_host.size());
        event.data.putn(req.raw_path.data(), req.raw_path.size());
        event.data.putc('?');
        event.data.putn(req.raw_query.data(), req.raw_query.size());

        this->do_push_event_common(move(event));
        return http_payload_normal;
      }

    virtual
    void
    do_on_wss_accepted(cow_string&& caddr) override
      {
        Event event;
        event.type = easy_hws_open;
        event.data.putn(caddr.data(), caddr.size());
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_wss_message_finish(WS_Opcode opcode, linear_buffer&& data) override
      {
        Easy_HWS_Event ev_type;
        if(opcode == ws_TEXT)
          ev_type = easy_hws_text;
        else if(opcode == ws_BINARY)
          ev_type = easy_hws_binary;
        else if(opcode == ws_PONG)
          ev_type = easy_hws_pong;
        else
          return;

        Event event;
        event.type = ev_type;
        event.data.swap(data);
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_wss_close(WS_Status status, chars_view reason) override
      {
        tinyfmt_ln fmt;
        fmt << status << ": " << reason;

        Event event;
        event.type = easy_hws_close;
        event.data = fmt.extract_buffer();
        this->do_push_event_common(move(event));
      }
  };

struct Final_Acceptor final : TCP_Acceptor
  {
    Easy_HWSS_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Acceptor(const IPv6_Address& addr,
                   const Easy_HWSS_Server::callback_type& callback,
                   const shptr<Session_Table>& sessions)
      :
        TCP_Acceptor(addr),
        m_callback(callback), m_wsessions(sessions)
      {
      }

    virtual
    shptr<Abstract_Socket>
    do_accept_socket_opt(IPv6_Address&& addr, unique_posix_fd&& fd) override
      {
        auto sessions = this->m_wsessions.lock();
        if(!sessions)
          return nullptr;

        auto session = new_sh<Final_Session>(move(fd), this->m_callback, sessions);
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

POSEIDON_HIDDEN_X_STRUCT(Easy_HWSS_Server,
  Session_Table);

Easy_HWSS_Server::
~Easy_HWSS_Server()
  {
  }

const IPv6_Address&
Easy_HWSS_Server::
local_address() const noexcept
  {
    if(!this->m_acceptor)
      return ipv6_unspecified;

    return this->m_acceptor->local_address();
  }

shptr<TCP_Acceptor>
Easy_HWSS_Server::
start(const IPv6_Address& addr, const callback_type& callback)
  {
    auto sessions = new_sh<X_Session_Table>();
    auto acceptor = new_sh<Final_Acceptor>(addr, callback, sessions);

    network_driver.insert(acceptor);
    this->m_sessions = move(sessions);
    this->m_acceptor = acceptor;
    return acceptor;
  }

shptr<TCP_Acceptor>
Easy_HWSS_Server::
start(const cow_string& addr, const callback_type& callback)
  {
    return this->start(IPv6_Address(addr), callback);
  }

shptr<TCP_Acceptor>
Easy_HWSS_Server::
start_any(uint16_t port, const callback_type& callback)
  {
    return this->start(IPv6_Address(ipv6_unspecified, port), callback);
  }

void
Easy_HWSS_Server::
stop() noexcept
  {
    this->m_sessions = nullptr;
    this->m_acceptor = nullptr;
  }

}  // namespace poseidon
