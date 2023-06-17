// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_ssl_server.hpp"
#include "../socket/listen_socket.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Client_Table
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
            Connection_Event type;
            linear_buffer data;
          };

        deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    unordered_map<const volatile SSL_Socket*, Event_Queue> client_map;
  };

struct Shared_cb_args
  {
    wkptr<void> wobj;
    callback_thunk_ptr<shptrR<SSL_Socket>, Abstract_Fiber&, Connection_Event, linear_buffer&> thunk;
    wkptr<Client_Table> wtable;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Shared_cb_args m_cb;
    const volatile SSL_Socket* m_refptr;

    explicit
    Final_Fiber(const Shared_cb_args& cb, const volatile SSL_Socket* refptr)
      : m_cb(cb), m_refptr(refptr)
      { }

    virtual
    void
    do_abstract_fiber_on_work()
      {
        auto cb_obj = this->m_cb.wobj.lock();
        if(!cb_obj)
          return;

        for(;;) {
          // The event callback may stop this server, so we have to check for
          // expiry in every iteration.
          auto table = this->m_cb.wtable.lock();
          if(!table)
            return;

          // Pop an event and invoke the user-defined callback here in the
          // main thread. Exceptions are ignored.
          plain_mutex::unique_lock lock(table->mutex);

          auto client_iter = table->client_map.find(this->m_refptr);
          if(client_iter == table->client_map.end())
            return;

          if(client_iter->second.events.empty()) {
            // Terminate now.
            client_iter->second.fiber_active = false;
            return;
          }

          // After `table->mutex` is unlocked, other threads may modify
          // `table->client_map` and invalidate all iterators, so maintain a
          // reference outside it for safety.
          auto queue = &(client_iter->second);
          ROCKET_ASSERT(queue->fiber_active);
          auto socket = queue->socket;
          auto event = ::std::move(queue->events.front());
          queue->events.pop_front();

          if(ROCKET_UNEXPECT(event.type == connection_event_closed)) {
            // This will be the last event on this socket.
            queue = nullptr;
            table->client_map.erase(client_iter);
          }
          client_iter = table->client_map.end();
          lock.unlock();

          try {
            if(event.type == connection_event_stream) {
              // `connection_event_stream` is special. We append new data to
              // `data_stream` which is then passed to the callback instead of
              // `event.data`. `data_stream` may be consumed partially by user
              // code, and shall be preserved across callbacks.
              queue->data_stream.putn(event.data.data(), event.data.size());
              this->m_cb.thunk(cb_obj.get(), socket, *this, event.type, queue->data_stream);
            }
            else
              this->m_cb.thunk(cb_obj.get(), socket, *this, event.type, event.data);
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
    Shared_cb_args m_cb;

    explicit
    Final_SSL_Socket(unique_posix_fd&& fd, const Shared_cb_args& cb)
      : SSL_Socket(::std::move(fd), network_driver.default_server_ssl_ctx()), m_cb(cb)
      { }

    void
    do_push_event_common(Connection_Event type, linear_buffer&& data) const
      {
        auto table = this->m_cb.wtable.lock();
        if(!table)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto client_iter = table->client_map.find(this);
        if(client_iter == table->client_map.end())
          return;

        if(!client_iter->second.fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_private_buffer` if no event is pending.
          fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_cb, this));
          client_iter->second.fiber_active = true;
        }

        auto& event = client_iter->second.events.emplace_back();
        event.type = type;
        event.data = ::std::move(data);
      }

    virtual
    void
    do_on_ssl_connected() override
      {
        linear_buffer data;
        this->do_push_event_common(connection_event_open, ::std::move(data));
      }

    virtual
    void
    do_on_ssl_stream(linear_buffer& data) override
      {
        this->do_push_event_common(connection_event_stream, ::std::move(data));
        data.clear();
      }

    virtual
    void
    do_abstract_socket_on_closed() override
      {
        linear_buffer data;
        if(errno != 0) {
          char sbuf[1024];
          data.puts(::strerror_r(errno, sbuf, sizeof(sbuf)));
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
    shptr<Abstract_Socket>
    do_on_listen_new_client_opt(Socket_Address&& addr, unique_posix_fd&& fd) override
      {
        auto table = this->m_cb.wtable.lock();
        if(!table)
          return nullptr;

        auto socket = new_sh<Final_SSL_Socket>(::std::move(fd), this->m_cb);
        (void) addr;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto r = table->client_map.try_emplace(socket.get());
        ROCKET_ASSERT(r.second);
        r.first->second.socket = socket;

        // Take its ownership.
        return socket;
      }
  };

}  // namespace

struct Easy_SSL_Server::X_Client_Table : Client_Table
  {
  };

Easy_SSL_Server::
~Easy_SSL_Server()
  {
  }

void
Easy_SSL_Server::
start(const Socket_Address& addr)
  {
    auto table = new_sh<X_Client_Table>();
    Shared_cb_args cb = { this->m_cb_obj, this->m_cb_thunk, table };
    auto socket = new_sh<Final_Listen_Socket>(addr, ::std::move(cb));

    network_driver.insert(socket);
    this->m_client_table = ::std::move(table);
    this->m_socket = ::std::move(socket);
  }

void
Easy_SSL_Server::
stop() noexcept
  {
    this->m_client_table = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_SSL_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

}  // namespace poseidon
