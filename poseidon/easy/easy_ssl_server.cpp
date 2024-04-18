// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_ssl_server.hpp"
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
        shptr<SSL_Socket> socket;
        cacheline_barrier xcb_1;

        // fiber-private fields; no locking needed
        linear_buffer data_stream;
        cacheline_barrier xcb_2;

        // shared fields between threads
        struct Event
          {
            Easy_Stream_Event type;
            linear_buffer data;
            int code = 0;
          };

        deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    unordered_map<const volatile SSL_Socket*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_SSL_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    const volatile SSL_Socket* m_refptr;

    Final_Fiber(const Easy_SSL_Server::callback_type& callback,
                shptrR<Session_Table> sessions,
                const volatile SSL_Socket* refptr)
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
            POSEIDON_LOG_ERROR(("Unhandled exception thrown fromt: $1"), stdex);
            socket->quick_close();
          }
        }
      }
  };

struct Final_Socket final : SSL_Socket
  {
    Easy_SSL_Server::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Socket(const Easy_SSL_Server::callback_type& callback,
                 unique_posix_fd&& fd, shptrR<Session_Table> sessions)
      :
        SSL_Socket(move(fd), network_driver),
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
    do_on_ssl_connected() override
      {
        Session_Table::Event_Queue::Event event;
        event.type = easy_stream_open;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ssl_stream(linear_buffer& data, bool eof) override
      {
        Session_Table::Event_Queue::Event event;
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

        Session_Table::Event_Queue::Event event;
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

    Final_Acceptor(const Easy_SSL_Server::callback_type& callback,
                   const IPv6_Address& addr, shptrR<Session_Table> sessions)
      :
        TCP_Acceptor(addr),
        m_callback(callback), m_wsessions(sessions)
      { }

    virtual
    shptr<Abstract_Socket>
    do_accept_socket_opt(const IPv6_Address& addr, unique_posix_fd&& fd) override
      {
        auto sessions = this->m_wsessions.lock();
        if(!sessions)
          return nullptr;

        auto socket = new_sh<Final_Socket>(this->m_callback, move(fd), sessions);
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

shptr<TCP_Acceptor>
Easy_SSL_Server::
start(chars_view addr)
  {
    // Parse the listen address string.
    IPv6_Address saddr;
    if(saddr.parse(addr) != addr.n)
      POSEIDON_THROW(("Invalid local IP address `$1`"), addr);

    // Initiate the server.
    auto sessions = new_sh<X_Session_Table>();
    auto acceptor = new_sh<Final_Acceptor>(this->m_callback, saddr, sessions);

    network_driver.insert(acceptor);
    this->m_sessions = move(sessions);
    this->m_acceptor = acceptor;
    return acceptor;
  }

void
Easy_SSL_Server::
stop() noexcept
  {
    this->m_sessions = nullptr;
    this->m_acceptor = nullptr;
  }

}  // namespace poseidon
