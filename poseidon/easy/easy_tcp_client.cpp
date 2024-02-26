// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_tcp_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../socket/async_connect.hpp"
#include "../static/async_task_executor.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<TCP_Socket> wsocket;
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

    mutable plain_mutex mutex;
    deque<Event> events;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_TCP_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    Final_Fiber(const Easy_TCP_Client::thunk_type& thunk, shptrR<Event_Queue> queue)
      :
        m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_on_abstract_fiber_execute() override
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
          auto event = move(queue->events.front());
          queue->events.pop_front();
          lock.unlock();

          try {
            // `easy_stream_data` is really special. We append new data to
            // `data_stream` which is passed to the callback instead of
            // `event.data`. `data_stream` may be consumed partially by user code,
            // and shall be preserved across callbacks.
            if(event.type == easy_stream_data)
              this->m_thunk(socket, *this, event.type,
                        splice_buffers(queue->data_stream, move(event.data)), event.code);
            else
              this->m_thunk(socket, *this, event.type, event.data, event.code);
          }
          catch(exception& stdex) {
            // Shut the connection down asynchronously. Pending output data
            // are discarded, but the user-defined callback will still be called
            // for remaining input data, in case there is something useful.
            socket->quick_close();

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy TCP client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_Socket final : TCP_Socket
  {
    Easy_TCP_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    Final_Socket(const Easy_TCP_Client::thunk_type& thunk, shptrR<Event_Queue> queue)
      :
        m_thunk(thunk), m_wqueue(queue)
      { }

    void
    do_push_event_common(Event_Queue::Event&& event)
      {
        auto queue = this->m_wqueue.lock();
        if(!queue)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(queue->mutex);

        try {
          if(!queue->fiber_active) {
            // Create a new fiber, if none is active. The fiber shall only reset
            // `m_fiber_private_buffer` if no event is pending.
            fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_thunk, queue));
            queue->fiber_active = true;
          }

          queue->events.push_back(move(event));
        }
        catch(exception& stdex) {
          POSEIDON_LOG_ERROR((
              "Could not push network event: $1"),
              stdex);

          this->quick_close();
        }
      }

    virtual
    void
    do_on_tcp_connected() override
      {
        Event_Queue::Event event;
        event.type = easy_stream_open;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_tcp_stream(linear_buffer& data, bool eof) override
      {
        Event_Queue::Event event;
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

        Event_Queue::Event event;
        event.type = easy_stream_close;
        event.data.puts(err_str);
        event.code = err_code;
        this->do_push_event_common(move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_TCP_Client, Event_Queue);

Easy_TCP_Client::
~Easy_TCP_Client()
  {
  }

void
Easy_TCP_Client::
connect(chars_view addr)
  {
    // Parse the address string, which shall contain a host name and an optional
    // port to connect. A port is required.
    Network_Reference caddr;
    if(parse_network_reference(caddr, addr) != addr.n)
      POSEIDON_THROW(("Invalid connect address `$1`"), addr);

    if(caddr.port.n == 0)
      POSEIDON_THROW(("No port specified in connect address `$1`"), addr);

    // Disallow superfluous components.
    if(caddr.path.p != nullptr)
      POSEIDON_THROW(("URI paths shall not be specified in connect address `$1`"), addr);

    if(caddr.query.p != nullptr)
      POSEIDON_THROW(("URI queries shall not be specified in connect address `$1`"), addr);

    if(caddr.fragment.p != nullptr)
      POSEIDON_THROW(("URI fragments shall not be specified in connect address `$1`"), addr);

    // Initiate the connection.
    auto queue = new_sh<X_Event_Queue>();
    auto socket = new_sh<Final_Socket>(this->m_thunk, queue);

    queue->wsocket = socket;
    auto dns_task = new_sh<Async_Connect>(network_driver, socket,
                       cow_string(caddr.host), caddr.port_num);

    async_task_executor.enqueue(dns_task);
    this->m_dns_task = move(dns_task);
    this->m_queue = move(queue);
    this->m_socket = move(socket);
  }

void
Easy_TCP_Client::
close() noexcept
  {
    this->m_dns_task = nullptr;
    this->m_queue = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_TCP_Client::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

const Socket_Address&
Easy_TCP_Client::
remote_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->remote_address();
  }

bool
Easy_TCP_Client::
tcp_send(chars_view data)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->tcp_send(data);
  }

bool
Easy_TCP_Client::
tcp_shut_down() noexcept
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->tcp_shut_down();
  }

}  // namespace poseidon
