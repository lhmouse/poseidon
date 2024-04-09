// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_hws_server.hpp"
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
        shptr<WS_Server_Session> session;
        cacheline_barrier xcb_1;

        // shared fields between threads
        struct Event
          {
            Easy_HWS_Event type;
            linear_buffer data;
          };

        deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    unordered_map<const volatile WS_Server_Session*, Event_Queue> client_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HWS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;
    const volatile WS_Server_Session* m_refptr;

    Final_Fiber(const Easy_HWS_Server::thunk_type& thunk, shptrR<Client_Table> table,
                const volatile WS_Server_Session* refptr)
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

          if(ROCKET_UNEXPECT(event.type == easy_hws_close)) {
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
            POSEIDON_LOG_ERROR(("Unhandled exception thrown from easy WS server: $1"), stdex);
            session->ws_shut_down(1015);
          }
        }
      }
  };

struct Final_Session final : WS_Server_Session
  {
    Easy_HWS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    Final_Session(const Easy_HWS_Server::thunk_type& thunk, unique_posix_fd&& fd,
                  shptrR<Client_Table> table)
      :
        TCP_Socket(move(fd)), m_thunk(thunk), m_wtable(table)
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
          POSEIDON_LOG_ERROR(("Could not push network event: $1"), stdex);
          table->client_map.erase(client_iter);
          this->quick_close();
        }
      }

    virtual
    HTTP_Payload_Type
    do_on_http_request_headers(HTTP_Request_Headers& req, bool close_after_payload) override
      {
        if(req.is_proxy) {
          // Reject proxy requests.
          this->do_on_http_request_error(HTTP_STATUS_FORBIDDEN);
          return http_payload_normal;
        }

        bool is_get = xstreq(req.method, "GET");
        bool is_head = xstreq(req.method, "HEAD");
        bool is_options = xstreq(req.method, "OPTIONS");

        if(!is_get && !is_head && !is_options) {
          // Reject the request.
          this->do_on_http_request_error(HTTP_STATUS_METHOD_NOT_ALLOWED);
          return http_payload_normal;
        }

        if(!is_options) {
          bool has_upgrade = false;
          for(const auto& r : req.headers)
            has_upgrade |= ascii_ci_equal(r.first, "Upgrade");

          if(!has_upgrade) {
            // Handle an HTTP request.
            Client_Table::Event_Queue::Event event;
            event.type = is_get ? easy_hws_get : easy_hws_head;
            event.data.putn(req.uri_host.data(), req.uri_host.size());
            event.data.putn(req.uri_path.data(), req.uri_path.size());
            event.data.putc('?');
            event.data.putn(req.uri_query.data(), req.uri_query.size());
            this->do_push_event_common(move(event));
            return http_payload_normal;
          }
        }

        // default
        return this->WS_Server_Session::do_on_http_request_headers(req, close_after_payload);
      }

    virtual
    void
    do_on_ws_accepted(cow_string&& caddr) override
      {
        Client_Table::Event_Queue::Event event;
        event.type = easy_hws_open;
        event.data.putn(caddr.data(), caddr.size());
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ws_message_finish(WebSocket_OpCode opcode, linear_buffer&& data) override
      {
        Easy_HWS_Event ev_type;
        if(opcode == websocket_text)
          ev_type = easy_hws_text;
        else if(opcode == websocket_binary)
          ev_type = easy_hws_binary;
        else if(opcode == websocket_pong)
          ev_type = easy_hws_pong;
        else
          return;

        Client_Table::Event_Queue::Event event;
        event.type = ev_type;
        event.data.swap(data);
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_ws_close(uint16_t status, chars_view reason) override
      {
        tinyfmt_ln fmt;
        fmt << status << ": " << reason;

        Client_Table::Event_Queue::Event event;
        event.type = easy_hws_close;
        event.data = fmt.extract_buffer();
        this->do_push_event_common(move(event));
      }
  };

struct Final_Listener final : Listen_Socket
  {
    Easy_HWS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    Final_Listener(const Easy_HWS_Server::thunk_type& thunk, const IPv6_Address& addr,
                   shptrR<Client_Table> table)
      :
        Listen_Socket(addr), m_thunk(thunk), m_wtable(table)
      {
        this->defer_accept(10s);
      }

    virtual
    shptr<Abstract_Socket>
    do_on_listen_new_client_opt(IPv6_Address&& addr, unique_posix_fd&& fd) override
      {
        auto table = this->m_wtable.lock();
        if(!table)
          return nullptr;

        auto session = new_sh<Final_Session>(this->m_thunk, move(fd), table);
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

POSEIDON_HIDDEN_X_STRUCT(Easy_HWS_Server, Client_Table);

Easy_HWS_Server::
~Easy_HWS_Server()
  {
  }

void
Easy_HWS_Server::
start(chars_view addr)
  {
    IPv6_Address saddr(addr);
    auto table = new_sh<X_Client_Table>();
    auto socket = new_sh<Final_Listener>(this->m_thunk, saddr, table);

    network_driver.insert(socket);
    this->m_client_table = move(table);
    this->m_socket = move(socket);
  }

void
Easy_HWS_Server::
stop() noexcept
  {
    this->m_client_table = nullptr;
    this->m_socket = nullptr;
  }

const IPv6_Address&
Easy_HWS_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

}  // namespace poseidon

