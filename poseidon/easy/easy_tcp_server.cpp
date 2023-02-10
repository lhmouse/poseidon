// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_tcp_server.hpp"
#include "../socket/tcp_socket.hpp"
#include "../socket/listen_socket.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../static/async_logger.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

using my_thunk = void (void*, shared_ptrR<TCP_Socket>, Connection_Event, linear_buffer&);

struct Client_Table
  {
    mutable plain_mutex mutex;
    weak_ptr<Listen_Socket> wsocket;  // read-only; no locking needed

    struct Event_Queue
      {
        struct Event
          {
            Connection_Event type;
            linear_buffer data;
          };

        shared_ptr<TCP_Socket> client;  // read-only; no locking needed
        linear_buffer fiber_private_buffer;  // by fibers only; no locking needed

        deque<Event> events;
        bool fiber_active = false;
      };

    unordered_map<const volatile TCP_Socket*, Event_Queue> clients;
  };

struct Shared_cb_args
  {
    weak_ptr<void> wobj;
    my_thunk* thunk;
    weak_ptr<Client_Table> wtable;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Shared_cb_args m_cb;
    const volatile TCP_Socket* m_key;

    explicit
    Final_Fiber(const Shared_cb_args& cb, const volatile TCP_Socket* key)
      : m_cb(cb), m_key(key)
      { }

    virtual
    void
    do_abstract_fiber_on_work()
      {
        for(;;) {
          auto cb_obj = this->m_cb.wobj.lock();
          if(!cb_obj)
            return;

          // The event callback may stop this server, so we have to check for
          // expiry in every iteration.
          auto table = this->m_cb.wtable.lock();
          if(!table)
            return;

          // We are in the main thread here.
          plain_mutex::unique_lock lock(table->mutex);

          auto citer = table->clients.find(this->m_key);
          if(citer == table->clients.end())
            return;

          if(citer->second.events.empty()) {
            // Leave now.
            citer->second.fiber_active = false;
            return;
          }

          ROCKET_ASSERT(citer->second.client.get() == this->m_key);
          ROCKET_ASSERT(citer->second.fiber_active);
          auto event = ::std::move(citer->second.events.front());
          citer->second.events.pop_front();
          auto client = citer->second.client;
          auto fiber_private_buffer = &(citer->second.fiber_private_buffer);

          if(event.type != connection_event_stream)
            fiber_private_buffer = nullptr;

          if(event.type == connection_event_closed)
            table->clients.erase(citer);

          lock.unlock();
          table = nullptr;

          try {
            // Invoke the user-defined event callback.
            if(event.type == connection_event_stream) {
              // `connection_event_stream` is special. We append new data to the
              // private buffer which is then passed to the callback instead of
              // `event.data`. The private buffer is preserved across callbacks
              // and may be consumed partially by user code.
              fiber_private_buffer->putn(event.data.data(), event.data.size());
              this->m_cb.thunk(cb_obj.get(), client, event.type, *fiber_private_buffer);
            }
            else
              this->m_cb.thunk(cb_obj.get(), client, event.type, event.data);
          }
          catch(exception& stdex) {
            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy TCP server: $1"),
                stdex);

            client->quick_shut_down();
            continue;
          }
        }
      }
  };

struct Final_TCP_Socket final : TCP_Socket
  {
    Shared_cb_args m_cb;

    explicit
    Final_TCP_Socket(unique_posix_fd&& fd, const Shared_cb_args& cb)
      : TCP_Socket(::std::move(fd)), m_cb(cb)
      { }

    void
    do_push_event_common(Connection_Event type, linear_buffer&& data) const
      {
        auto table = this->m_cb.wtable.lock();
        if(!table)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto citer = table->clients.find(this);
        if(citer == table->clients.end())
          return;

        ROCKET_ASSERT(citer->second.client.get() == this);

        if(!citer->second.fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_private_buffer` if no event is pending.
          fiber_scheduler.insert(::std::make_unique<Final_Fiber>(this->m_cb, this));
          citer->second.fiber_active = true;
        }

        auto& event = citer->second.events.emplace_back();
        event.type = type;
        event.data = ::std::move(data);
      }

    virtual
    void
    do_on_tcp_connected() override
      {
        linear_buffer data;
        this->do_push_event_common(connection_event_open, ::std::move(data));
      }

    virtual
    void
    do_on_tcp_stream(linear_buffer& data) override
      {
        this->do_push_event_common(connection_event_stream, ::std::move(data));
        data.clear();
      }

    virtual
    void
    do_abstract_socket_on_closed(int err) override
      {
        linear_buffer data;
        if(err != 0) {
          char msg_buf[256];
          const char* msg = ::strerror_r(err, msg_buf, sizeof(msg_buf));
          data.puts(msg);
        }
        this->do_push_event_common(connection_event_closed, ::std::move(data));
      }
  };

struct Final_Listen_Socket final : Listen_Socket
  {
    Shared_cb_args m_cb;

    explicit
    Final_Listen_Socket(const Socket_Address& addr, Shared_cb_args&& cb)
      : Listen_Socket(addr), m_cb(::std::move(cb))
      { }

    virtual
    shared_ptr<Abstract_Socket>
    do_on_listen_new_client_opt(Socket_Address&& addr, unique_posix_fd&& fd) override
      {
        auto table = this->m_cb.wtable.lock();
        if(!table)
          return nullptr;

        // Create the client socket.
        auto client = ::std::make_shared<Final_TCP_Socket>(::std::move(fd), this->m_cb);
        (void) addr;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto& queue = table->clients[client.get()];
        queue.client = client;
        ROCKET_ASSERT(queue.events.empty());

        // Take ownership of the client socket.
        return client;
      }
  };

}  // namespace

struct Easy_TCP_Server::X_Client_Table : Client_Table
  {
  };

Easy_TCP_Server::
~Easy_TCP_Server()
  {
  }

void
Easy_TCP_Server::
start(const Socket_Address& addr)
  {
    auto table = ::std::make_shared<X_Client_Table>();
    Shared_cb_args cb = { this->m_cb_obj, this->m_cb_thunk, table };
    auto socket = ::std::make_shared<Final_Listen_Socket>(addr, ::std::move(cb));
    table->wsocket = socket;

    network_driver.insert(socket);
    this->m_client_table = ::std::move(table);
    this->m_socket = ::std::move(socket);
  }

void
Easy_TCP_Server::
stop() noexcept
  {
    this->m_client_table = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_TCP_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_unspecified;

    return this->m_socket->local_address();
  }

}  // namespace poseidon
