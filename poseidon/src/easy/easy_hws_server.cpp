// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../easy/easy_hws_server.hpp"
#include "../../socket/tcp_acceptor.hpp"
#include "../../static/network_scheduler.hpp"
#include "../../fiber/abstract_fiber.hpp"
#include "../../static/fiber_scheduler.hpp"
#include "../../utils.hpp"
#include <deque>
#include <unordered_map>
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
    shptr<WS_Server_Session> session;

    // shared fields between threads
    alignas(::std::hardware_destructive_interference_size) ::std::deque<Event> events;
    bool fiber_active = false;
  };

struct Session_Table
  {
    mutable plain_mutex mutex;
    uint64_t next_serial = 0;
    ::std::unordered_map<uint64_t, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HWS_Server::callback_type m_callback;
    wkptr<Session_Table> m_wtable;
    uint64_t m_serial;

    Final_Fiber(const Easy_HWS_Server::callback_type& callback,
          const shptr<Session_Table>& table, uint64_t serial)
      :
        m_callback(callback), m_wtable(table), m_serial(serial)
      { }

    virtual
    void
    do_on_abstract_fiber_execute() override
      {
        for(;;) {
          // The event callback may stop this server, so we have to check for
          // expiry in every iteration.
          auto table = this->m_wtable.lock();
          if(!table)
            return;

          // Pop an event and invoke the user-defined callback here in the
          // main thread. Exceptions are ignored.
          plain_mutex::unique_lock lock(table->mutex);

          auto session_iter = table->session_map.find(this->m_serial);
          if(session_iter == table->session_map.end())
            return;

          if(session_iter->second.events.empty()) {
            // Terminate now.
            session_iter->second.fiber_active = false;
            return;
          }

          // After `table->mutex` is unlocked, other threads may modify
          // `table->session_map` and invalidate all iterators, so maintain a
          // reference outside it for safety.
          ASTERIA_ASSERT(session_iter->second.fiber_active);
          auto session = session_iter->second.session;
          auto event = move(session_iter->second.events.front());

          if(ASTERIA_UNEXPECT(event.type != easy_hws_close))
            session_iter->second.events.pop_front();
          else
            table->session_map.erase(session_iter);

          session_iter = { };
          lock.unlock();

          // `table->mutex` has been unlocked, and we don't need to hold a
          // reference to the table anymore. This allows the table to be
          // destroyed.
          table.reset();

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

struct Final_Session final : WS_Server_Session
  {
    uint64_t m_serial;
    Easy_HWS_Server::callback_type m_callback;
    wkptr<Session_Table> m_wtable;

    Final_Session(unique_posix_fd&& fd, uint64_t serial,
          const Easy_HWS_Server::callback_type& callback,
          const shptr<Session_Table>& table)
      :
        TCP_Socket(move(fd)), WS_Server_Session(), m_serial(serial),
        m_callback(callback), m_wtable(table)
      { }

    void
    do_push_event_common(Event&& event)
      {
        auto table = this->m_wtable.lock();
        if(!table)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto session_iter = table->session_map.find(this->m_serial);
        if(session_iter == table->session_map.end())
          return;

        try {
          if(!session_iter->second.fiber_active) {
            // Create a new fiber, if none is active. The fiber shall only reset
            // `m_fiber_private_buffer` if no event is pending.
            fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_callback, table, this->m_serial));
            session_iter->second.fiber_active = true;
          }

          session_iter->second.events.push_back(move(event));
        }
        catch(exception& stdex) {
          POSEIDON_LOG_ERROR(("Could not push network event: $1"), stdex);
          table->session_map.erase(session_iter);
          this->quick_shut_down();
        }
      }

    virtual
    void
    do_on_http_request_headers(HTTP_Request_Headers& req, bool eot) override
      {
        if(req.is_proxy) {
          // Reject proxy requests.
          this->do_on_http_request_error(req.method, http_status_forbidden);
          return;
        }

        Event event;
        switch(req.method.as_number())
          {
          case http_method_OPTIONS:
            // Respond to an XSS check.
            this->do_ws_complete_handshake(req, eot);
            return;

          case http_method_GET:
            if(any_of(req.headers, [&](const auto& r) { return r.first == "Upgrade";  })) {
              // Try upgrading to WebSocket.
              this->do_ws_complete_handshake(req, eot);
              return;
            }

            // Handle a plain HTTP GET request.
            event.type = easy_hws_get;
            break;

          case http_method_HEAD:
            // Handle a plain HTTP HEAD request.
            event.type = easy_hws_head;
            break;

          default:
            // Reject all the other.
            this->do_on_http_request_error(req.method, http_status_method_not_allowed);
            this->quick_shut_down();
            return;
          }

        event.data.putn(req.raw_host.data(), req.raw_host.size());
        event.data.putn(req.raw_path.data(), req.raw_path.size());
        event.data.putc('?');
        event.data.putn(req.raw_query.data(), req.raw_query.size());

        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ws_accepted(cow_string&& caddr) override
      {
        Event event;
        event.type = easy_hws_open;
        event.data.putn(caddr.data(), caddr.size());
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ws_message_finish(WS_Opcode opcode, linear_buffer&& data) override
      {
        Easy_HWS_Event ev_type;
        if(opcode == ws_opcode_TEXT)
          ev_type = easy_hws_text;
        else if(opcode == ws_opcode_BINARY)
          ev_type = easy_hws_binary;
        else if(opcode == ws_opcode_PONG)
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
    do_on_ws_close(WS_Status status, chars_view reason) override
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
    Easy_HWS_Server::callback_type m_callback;
    wkptr<Session_Table> m_wtable;

    Final_Acceptor(const IPv6_Address& addr,
          const Easy_HWS_Server::callback_type& callback,
          const shptr<Session_Table>& table)
      :
        TCP_Acceptor(addr), m_callback(callback), m_wtable(table)
      {
      }

    virtual
    shptr<Abstract_Socket>
    do_accept_socket_opt(IPv6_Address&& /*addr*/, unique_posix_fd&& fd) override
      {
        auto table = this->m_wtable.lock();
        if(!table)
          return nullptr;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto session = new_sh<Final_Session>(move(fd), table->next_serial ++,
                                             this->m_callback, table);

        auto r = table->session_map.try_emplace(session->m_serial);
        ASTERIA_ASSERT(r.second);
        r.first->second.session = session;
        return session;
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_HWS_Server,
  Session_Table);

Easy_HWS_Server::
~Easy_HWS_Server()
  {
  }

const IPv6_Address&
Easy_HWS_Server::
local_address() const noexcept
  {
    if(!this->m_acceptor)
      return ipv6_unspecified;

    return this->m_acceptor->local_address();
  }

shptr<TCP_Acceptor>
Easy_HWS_Server::
start(const IPv6_Address& addr, const callback_type& callback)
  {
    auto table = new_sh<X_Session_Table>();
    auto acceptor = new_sh<Final_Acceptor>(addr, callback, table);

    network_scheduler.insert_weak(acceptor);
    this->m_table = move(table);
    this->m_acceptor = acceptor;
    return acceptor;
  }

shptr<TCP_Acceptor>
Easy_HWS_Server::
start(const cow_string& addr, const callback_type& callback)
  {
    return this->start(IPv6_Address(addr), callback);
  }

shptr<TCP_Acceptor>
Easy_HWS_Server::
start(uint16_t port, const callback_type& callback)
  {
    return this->start(IPv6_Address(ipv6_unspecified, port), callback);
  }

void
Easy_HWS_Server::
stop() noexcept
  {
    this->m_table = nullptr;
    this->m_acceptor = nullptr;
  }

}  // namespace poseidon
