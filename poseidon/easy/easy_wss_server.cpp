// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_wss_server.hpp"
#include "enums.hpp"
#include "../socket/listen_socket.hpp"
#include "../socket/enums.hpp"
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
        shptr<WSS_Server_Session> session;
        cacheline_barrier xcb_1;

        // shared fields between threads
        struct Event
          {
            Easy_WS_Event type;
            linear_buffer data;
          };

        deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    unordered_map<const volatile WSS_Server_Session*, Event_Queue> client_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_WSS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;
    const volatile WSS_Server_Session* m_refptr;

    explicit
    Final_Fiber(const Easy_WSS_Server::thunk_type& thunk, shptrR<Client_Table> table,
                const volatile WSS_Server_Session* refptr)
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
          auto session = queue->session;
          auto event = move(queue->events.front());
          queue->events.pop_front();

          if(ROCKET_UNEXPECT(event.type == easy_ws_close)) {
            // This will be the last event on this session.
            queue = nullptr;
            table->client_map.erase(client_iter);
          }
          client_iter = table->client_map.end();
          lock.unlock();

          try {
            // Process a message.
            this->m_thunk(session, *this, event.type, move(event.data));
          }
          catch(exception& stdex) {
            // Shut the connection down with a message.
            session->wss_shut_down(1015);

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy WSS server: $1"),
                stdex);
          }
        }
      }
  };

struct FInal_Server_Session final : WSS_Server_Session
  {
    Easy_WSS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    explicit
    FInal_Server_Session(unique_posix_fd&& fd,
          const Easy_WSS_Server::thunk_type& thunk, shptrR<Client_Table> table)
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
            fiber_scheduler.launch(new_uni<Final_Fiber>(this->m_thunk, table, this));
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
    do_on_wss_accepted(cow_string&& caddr) override
      {
        Client_Table::Event_Queue::Event event;
        event.type = easy_ws_open;
        event.data.putn(caddr.data(), caddr.size());
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_wss_message_finish(WebSocket_OpCode opcode, linear_buffer&& data) override
      {
        Client_Table::Event_Queue::Event event;

        if(opcode == websocket_text)
          event.type = easy_ws_text;
        else if(opcode == websocket_binary)
          event.type = easy_ws_binary;
        else if(opcode == websocket_pong)
          event.type = easy_ws_pong;
        else
          return;

        event.data.swap(data);
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_wss_close(uint16_t status, chars_view reason) override
      {
        Client_Table::Event_Queue::Event event;
        event.type = easy_ws_close;

        tinyfmt_ln fmt;
        fmt << status << ": " << reason;
        event.data = fmt.extract_buffer();

        this->do_push_event_common(move(event));
      }
  };

struct Final_Listen_Socket final : Listen_Socket
  {
    Easy_WSS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    explicit
    Final_Listen_Socket(const Easy_WSS_Server::thunk_type& thunk,
                        const Socket_Address& addr, shptrR<Client_Table> table)
      :
        Listen_Socket(addr), m_thunk(thunk), m_wtable(table)
      {
        this->defer_accept(10s);
      }

    virtual
    shptr<Abstract_Socket>
    do_on_listen_new_client_opt(Socket_Address&& addr, unique_posix_fd&& fd) override
      {
        auto table = this->m_wtable.lock();
        if(!table)
          return nullptr;

        auto session = new_sh<FInal_Server_Session>(move(fd), this->m_thunk, table);
        (void) addr;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(table->mutex);

        auto r = table->client_map.try_emplace(session.get());
        ROCKET_ASSERT(r.second);
        r.first->second.session = session;
        return session;
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_WSS_Server, Client_Table);

Easy_WSS_Server::
~Easy_WSS_Server()
  {
  }

void
Easy_WSS_Server::
start(chars_view addr)
  {
    Socket_Address saddr(addr);
    auto table = new_sh<X_Client_Table>();
    auto socket = new_sh<Final_Listen_Socket>(this->m_thunk, saddr, table);

    network_driver.insert(socket);
    this->m_client_table = move(table);
    this->m_socket = move(socket);
  }

void
Easy_WSS_Server::
stop() noexcept
  {
    this->m_client_table = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_WSS_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

}  // namespace poseidon

