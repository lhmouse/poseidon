// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_tcp_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../socket/dns_connect_task.hpp"
#include "../static/task_executor.hpp"
#include "../utils.hpp"
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
    shptr<TCP_Socket> socket;
    shptr<DNS_Connect_Task> dns_task;
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
    ::std::unordered_map<volatile TCP_Socket*, Event_Queue> session_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_TCP_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;
    volatile TCP_Socket* m_refptr;

    Final_Fiber(const Easy_TCP_Client::callback_type& callback,
                const shptr<Session_Table>& sessions,
                volatile TCP_Socket* refptr)
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

struct Final_Socket final : TCP_Socket
  {
    Easy_TCP_Client::callback_type m_callback;
    wkptr<Session_Table> m_wsessions;

    Final_Socket(const Easy_TCP_Client::callback_type& callback,
                 const shptr<Session_Table>& sessions)
      :
        TCP_Socket(),
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
    do_on_tcp_connected() override
      {
        Event event;
        event.type = easy_stream_open;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_tcp_stream(linear_buffer& data, bool eof) override
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

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_TCP_Client,
  Session_Table);

Easy_TCP_Client::
~Easy_TCP_Client()
  {
  }

shptr<TCP_Socket>
Easy_TCP_Client::
connect(const cow_string& addr, const callback_type& callback)
  {
    // Parse the address string, which shall contain a host name and an optional
    // port to connect. A port is required.
    Network_Reference caddr;
    if(parse_network_reference(caddr, addr) != addr.size())
      POSEIDON_THROW(("Invalid address `$1`"), addr);

    if(caddr.port.n == 0)
      POSEIDON_THROW(("No port specified in address `$1`"), addr);

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

    auto socket = new_sh<Final_Socket>(callback, this->m_sessions);
    auto dns_task = new_sh<DNS_Connect_Task>(network_driver,
                       socket, cow_string(caddr.host), caddr.port_num);

    // Initiate the connection.
    plain_mutex::unique_lock lock(this->m_sessions->mutex);

    task_executor.enqueue(dns_task);
    auto r = this->m_sessions->session_map.try_emplace(socket.get());
    ROCKET_ASSERT(r.second);
    r.first->second.socket = socket;
    r.first->second.dns_task = move(dns_task);
    return socket;
  }

void
Easy_TCP_Client::
close_all() noexcept
  {
    this->m_sessions = nullptr;
  }

}  // namespace poseidon
