// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_https_server.hpp"
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
        shptr<HTTPS_Server_Session> session;
        cacheline_barrier xcb_1;

        // shared fields between threads
        struct Event
          {
            Easy_Socket_Event type;
            HTTP_Request_Headers req;
            linear_buffer data;
            bool close_now = false;
            uint32_t status = 0;
          };

        deque<Event> events;
        bool fiber_active = false;
      };

    mutable plain_mutex mutex;
    unordered_map<const volatile HTTPS_Server_Session*, Event_Queue> client_map;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HTTPS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;
    const volatile HTTPS_Server_Session* m_refptr;

    explicit
    Final_Fiber(const Easy_HTTPS_Server::thunk_type& thunk,
          const shptr<Client_Table>& table, const volatile HTTPS_Server_Session* refptr)
      : m_thunk(thunk), m_wtable(table), m_refptr(refptr)
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
          auto event = ::std::move(queue->events.front());
          queue->events.pop_front();

          if(ROCKET_UNEXPECT(event.type == easy_socket_close)) {
            // This will be the last event on this session.
            queue = nullptr;
            table->client_map.erase(client_iter);
          }
          client_iter = table->client_map.end();
          lock.unlock();

          try {
            if(event.type == easy_socket_pong) {
              // Send a bad request response.
              HTTP_Response_Headers resp;
              resp.status = event.status;
              resp.headers.emplace_back(sref("Connection"), sref("close"));
              session->https_response(::std::move(resp), "");
            }
            else {
              // Process a request.
              this->m_thunk(session, *this, event.type, ::std::move(event.req), ::std::move(event.data));
            }

            if(event.close_now)
              session->ssl_shut_down();
          }
          catch(exception& stdex) {
            // Shut the connection down with a message.
            // XXX: The user-defined callback may have sent a response...?
            HTTP_Response_Headers resp;
            resp.status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
            resp.headers.emplace_back(sref("Connection"), sref("close"));
            session->https_response(::std::move(resp), "");

            session->ssl_shut_down();

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy HTTP client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_HTTPS_Server_Session final : HTTPS_Server_Session
  {
    Easy_HTTPS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    explicit
    Final_HTTPS_Server_Session(unique_posix_fd&& fd,
          const Easy_HTTPS_Server::thunk_type& thunk, const shptr<Client_Table>& table)
      : SSL_Socket(::std::move(fd)), m_thunk(thunk), m_wtable(table)
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

          client_iter->second.events.push_back(::std::move(event));
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
    do_on_https_request_finish(HTTP_Request_Headers&& req, linear_buffer&& data, bool close_now) override
      {
        Client_Table::Event_Queue::Event event;
        event.type = easy_socket_msg_bin;
        event.req = ::std::move(req);
        event.data = ::std::move(data);
        event.close_now = close_now;
        this->do_push_event_common(::std::move(event));
      }

    virtual
    void
    do_on_https_request_error(uint32_t status) override
      {
        Client_Table::Event_Queue::Event event;
        event.type = easy_socket_pong;
        event.status = status;
        event.close_now = true;
        this->do_push_event_common(::std::move(event));
      }

    virtual
    void
    do_abstract_socket_on_closed() override
      {
        char sbuf[1024];
        int err_code = errno;
        const char* err_str = ::strerror_r(err_code, sbuf, sizeof(sbuf));

        Client_Table::Event_Queue::Event event;
        event.type = easy_socket_close;
        event.data.puts(err_str);
        this->do_push_event_common(::std::move(event));
      }
  };

struct Final_Listen_Socket final : Listen_Socket
  {
    Easy_HTTPS_Server::thunk_type m_thunk;
    wkptr<Client_Table> m_wtable;

    explicit
    Final_Listen_Socket(const Socket_Address& addr,
          const Easy_HTTPS_Server::thunk_type& thunk, const shptr<Client_Table>& table)
      : Listen_Socket(addr), m_thunk(thunk), m_wtable(table)
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

        auto session = new_sh<Final_HTTPS_Server_Session>(::std::move(fd), this->m_thunk, table);
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

POSEIDON_HIDDEN_X_STRUCT(Easy_HTTPS_Server, Client_Table);

Easy_HTTPS_Server::
~Easy_HTTPS_Server()
  {
  }

void
Easy_HTTPS_Server::
start(chars_view addr)
  {
    auto table = new_sh<X_Client_Table>();
    auto socket = new_sh<Final_Listen_Socket>(Socket_Address(addr), this->m_thunk, table);

    network_driver.insert(socket);
    this->m_client_table = ::std::move(table);
    this->m_socket = ::std::move(socket);
  }

void
Easy_HTTPS_Server::
stop() noexcept
  {
    this->m_client_table = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_HTTPS_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

}  // namespace poseidon
