// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
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
            Easy_Stream_Event type;
            linear_buffer data;
            int code = 0;
          };

        deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    unordered_map<const volatile SSL_Socket*, Event_Queue> client_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_SSL_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;
    const volatile SSL_Socket* m_refptr;

    Final_Fiber(const Easy_SSL_Server::thunk_type& thunk, shptrR<Client_Table> table,
                const volatile SSL_Socket* refptr)
      :
        m_thunk(thunk), m_wtable(table), m_refptr(refptr)
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
          auto event = move(queue->events.front());
          queue->events.pop_front();

          if(ROCKET_UNEXPECT(event.type == easy_stream_close)) {
            // This will be the last event on this socket.
            queue = nullptr;
            table->client_map.erase(client_iter);
          }
          client_iter = table->client_map.end();
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
                "Unhandled exception thrown from easy SSL client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_Socket final : SSL_Socket
  {
    Easy_SSL_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    Final_Socket(const Easy_SSL_Server::thunk_type& thunk, unique_posix_fd&& fd,
                 shptrR<Client_Table> table)
      :
        SSL_Socket(move(fd), network_driver), m_thunk(thunk), m_wtable(table)
      { }

    void
    do_push_event_common(Client_Table::Event_Queue::Event&& event)
      {
        auto table = this->m_wtable.lock();
        if(!table)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto client_iter = table->client_map.find(this);
        if(client_iter == table->client_map.end())
          return;

        try {
          if(!client_iter->second.fiber_active) {
            // Create a new fiber, if none is active. The fiber shall only reset
            // `m_fiber_private_buffer` if no event is pending.
            fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_thunk, table, this));
            client_iter->second.fiber_active = true;
          }

          client_iter->second.events.push_back(move(event));
        }
        catch(exception& stdex) {
          POSEIDON_LOG_ERROR((
              "Could not push network event: $1"),
              stdex);

          table->client_map.erase(client_iter);
          this->quick_close();
        }
      }

    virtual
    void
    do_on_ssl_connected() override
      {
        Client_Table::Event_Queue::Event event;
        event.type = easy_stream_open;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ssl_stream(linear_buffer& data, bool eof) override
      {
        Client_Table::Event_Queue::Event event;
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

        Client_Table::Event_Queue::Event event;
        event.type = easy_stream_close;
        event.data.puts(err_str);
        event.code = err_code;
        this->do_push_event_common(move(event));
      }
  };

struct Final_Listener final : Listen_Socket
  {
    Easy_SSL_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    Final_Listener(const Easy_SSL_Server::thunk_type& thunk, const IPv6_Address& addr,
                   shptrR<Client_Table> table)
      :
        Listen_Socket(addr), m_thunk(thunk), m_wtable(table)
      { }

    virtual
    shptr<Abstract_Socket>
    do_on_listen_new_client_opt(IPv6_Address&& addr, unique_posix_fd&& fd) override
      {
        auto table = this->m_wtable.lock();
        if(!table)
          return nullptr;

        auto socket = new_sh<Final_Socket>(this->m_thunk, move(fd), table);
        (void) addr;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto r = table->client_map.try_emplace(socket.get());
        ROCKET_ASSERT(r.second);
        r.first->second.socket = socket;
        return socket;
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_SSL_Server, Client_Table);

Easy_SSL_Server::
~Easy_SSL_Server()
  {
  }

void
Easy_SSL_Server::
start(chars_view addr)
  {
    // Parse the listen address string.
    IPv6_Address saddr(addr);

    // Initiate the server.
    auto table = new_sh<X_Client_Table>();
    auto socket = new_sh<Final_Listener>(this->m_thunk, saddr, table);

    network_driver.insert(socket);
    this->m_client_table = move(table);
    this->m_socket = move(socket);
  }

void
Easy_SSL_Server::
stop() noexcept
  {
    this->m_client_table = nullptr;
    this->m_socket = nullptr;
  }

const IPv6_Address&
Easy_SSL_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

}  // namespace poseidon
