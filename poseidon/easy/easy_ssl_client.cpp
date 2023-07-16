// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_ssl_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<SSL_Socket> wsocket;
    cacheline_barrier xcb_1;

    // fiber-private fields; no locking needed
    linear_buffer data_stream;
    cacheline_barrier xcb_2;

    // shared fields between threads
    struct Event
      {
        Connection_Event type;
        linear_buffer data;
        int code = 0;
      };

    mutable plain_mutex mutex;
    deque<Event> events;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_SSL_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_Fiber(const Easy_SSL_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
      : m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_abstract_fiber_on_work()
      {
        for(;;) {
          // The event callback may stop this client, so we have to check for
          // expiry in every iteration.
          auto queue = this->m_wqueue.lock();
          if(!queue)
            return;

          auto socket = queue->wsocket.lock();
          if(!socket)
            return;

          // Pop an event and invoke the user-defined callback here in the
          // main thread. Exceptions are ignored.
          plain_mutex::unique_lock lock(queue->mutex);

          if(queue->events.empty()) {
            // Terminate now.
            queue->fiber_active = false;
            return;
          }

          ROCKET_ASSERT(queue->fiber_active);
          auto event = ::std::move(queue->events.front());
          queue->events.pop_front();
          lock.unlock();

          try {
            if(event.type == connection_stream) {
              // `connection_stream` is special. We append new data to
              // `data_stream` which is then passed to the callback instead of
              // `event.data`. `data_stream` may be consumed partially by user
              // code, and shall be preserved across callbacks.
              queue->data_stream.putn(event.data.data(), event.data.size());
              this->m_thunk(socket, *this, event.type, queue->data_stream, event.code);
            }
            else
              this->m_thunk(socket, *this, event.type, event.data, event.code);
          }
          catch(exception& stdex) {
            // Shut the connection down asynchronously. Pending output data
            // are discarded, but the user-defined callback will still be called
            // for remaining input data, in case there is something useful.
            socket->quick_close();

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy SSL client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_SSL_Socket final : SSL_Socket
  {
    Easy_SSL_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_SSL_Socket(const Easy_SSL_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
      : m_thunk(thunk), m_wqueue(queue)
      { }

    void
    do_push_event_common(Event_Queue::Event&& event) const
      {
        auto queue = this->m_wqueue.lock();
        if(!queue)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(queue->mutex);

        if(!queue->fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_private_buffer` if no event is pending.
          fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_thunk, queue));
          queue->fiber_active = true;
        }

        queue->events.push_back(::std::move(event));
      }

    virtual
    void
    do_on_ssl_connected() override
      {
        Event_Queue::Event event;
        event.type = connection_open;
        this->do_push_event_common(::std::move(event));
      }

    virtual
    void
    do_on_ssl_stream(linear_buffer& data, bool eof) override
      {
        Event_Queue::Event event;
        event.type = connection_stream;
        event.data.swap(data);
        event.code = eof;
        this->do_push_event_common(::std::move(event));
      }

    virtual
    void
    do_abstract_socket_on_closed() override
      {
        char sbuf[1024];
        int err_code = errno;
        const char* err_str = ::strerror_r(err_code, sbuf, sizeof(sbuf));

        Event_Queue::Event event;
        event.type = connection_closed;
        event.data.puts(err_str);
        event.code = err_code;
        this->do_push_event_common(::std::move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_SSL_Client, Event_Queue);

Easy_SSL_Client::
~Easy_SSL_Client()
  {
  }

void
Easy_SSL_Client::
connect(const Socket_Address& addr)
  {
    auto queue = new_sh<X_Event_Queue>();
    auto socket = new_sh<Final_SSL_Socket>(this->m_thunk, queue);
    queue->wsocket = socket;
    socket->connect(addr);

    network_driver.insert(socket);
    this->m_queue = ::std::move(queue);
    this->m_socket = ::std::move(socket);
  }

void
Easy_SSL_Client::
close() noexcept
  {
    this->m_queue = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_SSL_Client::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

const Socket_Address&
Easy_SSL_Client::
remote_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->remote_address();
  }

bool
Easy_SSL_Client::
ssl_send(chars_proxy data)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->ssl_send(data);
  }

bool
Easy_SSL_Client::
ssl_shut_down() noexcept
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->ssl_shut_down();
  }

}  // namespace poseidon
