// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_ssl_server.hpp"
#include "../socket/tcp_acceptor.hpp"
#include "../static/network_scheduler.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
#include <deque>
#include <unordered_map>
namespace poseidon {
namespace {

struct Event
  {
    Easy_Stream_Event type;
    linear_buffer data;
    int code = 0;
  };

struct Event_Queue
  {
    // read-only fields; no locking needed
    shptr<SSL_Socket> socket;
    cacheline_barrier xcb_1;

    // fiber-private fields; no locking needed
    linear_buffer data_stream;
    cacheline_barrier xcb_2;

    // shared fields between threads
    ::std::deque<Event> events;
    bool fiber_active = false;
  };

struct Session_Table
  {
    mutable plain_mutex mutex;
    ::std::unordered_map<volatile SSL_Socket*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_SSL_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    volatile SSL_Socket* m_refptr;

    Final_Fiber(const Easy_SSL_Server::callback_type& callback,
                const shptr<Session_Table>& sessions, volatile SSL_Socket* refptr)
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
          auto socket = queue->socket;
          auto event = move(queue->events.front());
          queue->events.pop_front();

          if(ROCKET_UNEXPECT(event.type == easy_stream_close)) {
            // This will be the last event on this socket.
            queue = nullptr;
            sessions->session_map.erase(session_iter);
          }
          session_iter = sessions->session_map.end();
          lock.unlock();

          try {
            // `easy_stream_data` is really special. We append new data to
            // `data_stream` which is passed to the callback instead of
            // `event.data`. `data_stream` may be consumed partially by user code,
            // and shall be preserved across callbacks.
            if(event.type == easy_stream_data)
              this->m_callback(socket, *this, event.type,
                     splice_buffers(queue->data_stream, move(event.data)), event.code);
            else
              this->m_callback(socket, *this, event.type, event.data, event.code);
          }
          catch(exception& stdex) {
            // Shut the connection down asynchronously. Pending output data
            // are discarded, but the user-defined callback will still be called
            // for remaining input data, in case there is something useful.
            POSEIDON_LOG_ERROR(("Unhandled exception: $1"), stdex);
            socket->quick_shut_down();
          }
        }
      }
  };

struct Final_Socket final : SSL_Socket
  {
    Easy_SSL_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Socket(unique_posix_fd&& fd,
                 const Easy_SSL_Server::callback_type& callback,
                 const shptr<Session_Table>& sessions)
      :
        SSL_Socket(move(fd), network_scheduler),
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
          this->quick_shut_down();
        }
      }

    virtual
    void
    do_on_ssl_connected() override
      {
        Event event;
        event.type = easy_stream_open;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ssl_stream(linear_buffer& data, bool eof) override
      {
        Event event;
        event.type = easy_stream_data;
        event.data.swap(data);
        event.code = eof;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_abstract_socket_on_closed() override
      {
        char sbuf[1024];
        int err_code = errno;
        const char* err_str = ::strerror_r(err_code, sbuf, sizeof(sbuf));

        Event event;
        event.type = easy_stream_close;
        event.data.puts(err_str);
        event.code = err_code;
        this->do_push_event_common(move(event));
      }
  };

struct Final_Acceptor final : TCP_Acceptor
  {
    Easy_SSL_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Acceptor(const IPv6_Address& addr,
                   const Easy_SSL_Server::callback_type& callback,
                   const shptr<Session_Table>& sessions)
      :
        TCP_Acceptor(addr),
        m_callback(callback), m_wsessions(sessions)
      { }

    virtual
    shptr<Abstract_Socket>
    do_accept_socket_opt(IPv6_Address&& addr, unique_posix_fd&& fd) override
      {
        auto sessions = this->m_wsessions.lock();
        if(!sessions)
          return nullptr;

        auto socket = new_sh<Final_Socket>(move(fd), this->m_callback, sessions);
        (void) addr;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(sessions->mutex);

        auto r = sessions->session_map.try_emplace(socket.get());
        ROCKET_ASSERT(r.second);
        r.first->second.socket = socket;
        return socket;
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_SSL_Server,
  Session_Table);

Easy_SSL_Server::
~Easy_SSL_Server()
  {
  }

const IPv6_Address&
Easy_SSL_Server::
local_address() const noexcept
  {
    if(!this->m_acceptor)
      return ipv6_unspecified;

    return this->m_acceptor->local_address();
  }

shptr<TCP_Acceptor>
Easy_SSL_Server::
start(const IPv6_Address& addr, const callback_type& callback)
  {
    auto sessions = new_sh<X_Session_Table>();
    auto acceptor = new_sh<Final_Acceptor>(addr, callback, sessions);

    network_scheduler.insert_weak(acceptor);
    this->m_sessions = move(sessions);
    this->m_acceptor = acceptor;
    return acceptor;
  }

shptr<TCP_Acceptor>
Easy_SSL_Server::
start(const cow_string& addr, const callback_type& callback)
  {
    return this->start(IPv6_Address(addr), callback);
  }

shptr<TCP_Acceptor>
Easy_SSL_Server::
start(uint16_t port, const callback_type& callback)
  {
    return this->start(IPv6_Address(ipv6_unspecified, port), callback);
  }

void
Easy_SSL_Server::
stop() noexcept
  {
    this->m_sessions = nullptr;
    this->m_acceptor = nullptr;
  }

}  // namespace poseidon
