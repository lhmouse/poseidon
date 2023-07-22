// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_ws_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../socket/async_connect.hpp"
#include "../static/async_task_executor.hpp"
#include "../utils.hpp"
#include <http_parser.h>
namespace poseidon {
namespace {

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<WS_Client_Session> wsession;
    cacheline_barrier xcb_1;

    // fiber-private fields; no locking needed
    linear_buffer data_stream;
    cacheline_barrier xcb_2;

    // shared fields between threads
    struct Event
      {
        Easy_Socket_Event type;
        linear_buffer data;
      };

    mutable plain_mutex mutex;
    deque<Event> events;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_WS_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_Fiber(const Easy_WS_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
      : m_thunk(thunk), m_wqueue(queue)  { }

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

          auto session = queue->wsession.lock();
          if(!session)
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
            // Process a message.
            this->m_thunk(session, *this, event.type, ::std::move(event.data));
          }
          catch(exception& stdex) {
            // Shut the connection down with a message.
            session->ws_shut_down(1015);

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy TCP client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_WS_Client_Session final : WS_Client_Session
  {
    Easy_WS_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_WS_Client_Session(cow_stringR host, cow_stringR uri, cow_stringR query,
          const Easy_WS_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
      : WS_Client_Session(host, uri, query), m_thunk(thunk), m_wqueue(queue)  { }

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

          queue->events.push_back(::std::move(event));
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
    do_on_ws_connected(cow_string&& uri) override
      {
        Event_Queue::Event event;
        event.type = easy_socket_open;
        event.data.putn(uri.data(), uri.size());
        this->do_push_event_common(::std::move(event));
      }

    virtual
    void
    do_on_ws_message_finish(WebSocket_OpCode opcode, linear_buffer&& data) override
      {
        Event_Queue::Event event;

        if(opcode == websocket_text)
          event.type = easy_socket_msg_text;
        else if(opcode == websocket_bin)
          event.type = easy_socket_msg_bin;
        else if(opcode == websocket_pong)
          event.type = easy_socket_pong;
        else
          return;

        event.data.swap(data);
        this->do_push_event_common(::std::move(event));
      }

    virtual
    void
    do_on_ws_close(uint16_t status, chars_proxy reason) override
      {
        Event_Queue::Event event;
        event.type = easy_socket_close;

        tinyfmt_ln fmt;
        fmt << status << ": " << reason;
        event.data = fmt.extract_buffer();

        this->do_push_event_common(::std::move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_WS_Client, Event_Queue);

Easy_WS_Client::
~Easy_WS_Client()
  {
  }

void
Easy_WS_Client::
connect(cow_stringR uri)
  {
    // Parse the URI.
    ::http_parser_url uri_hp = { };
    if((uri.size() > UINT16_MAX) || (::http_parser_parse_url(uri.data(), uri.size(), false, &uri_hp)))
      POSEIDON_THROW(("URI `$1` not resolvable"), uri);

    if(!::rocket::ascii_ci_equal(uri.data() + uri_hp.field_data[UF_SCHEMA].off, uri_hp.field_data[UF_SCHEMA].len, "ws", 2))
      POSEIDON_THROW(("Protocol must be `ws://` (URI `$1`)"), uri);

    if(uri_hp.field_set & (1U << UF_USERINFO))
      POSEIDON_THROW(("User information not supported (URI `$1`)"), uri);

    // Set connection parameters. The host part is required; all the others have
    // default values.
    cow_string host = uri.substr(uri_hp.field_data[UF_HOST].off, uri_hp.field_data[UF_HOST].len);
    uint16_t port = 80;
    cow_string path = sref("/");
    cow_string query;

    if(uri_hp.field_set & (1U << UF_PORT))
      port = uri_hp.port;

    if(uri_hp.field_set & (1U << UF_PATH))
      path.assign(uri, uri_hp.field_data[UF_PATH].off, uri_hp.field_data[UF_PATH].len);

    if(uri_hp.field_set & (1U << UF_QUERY))
      query.assign(uri, uri_hp.field_data[UF_QUERY].off, uri_hp.field_data[UF_QUERY].len);

    // Initiate the connection.
    auto host_header = format_string("$1:$2", host, port);
    auto queue = new_sh<X_Event_Queue>();
    auto session = new_sh<Final_WS_Client_Session>(host_header, path, query, this->m_thunk, queue);
    queue->wsession = session;
    auto dns_task = new_sh<Async_Connect>(network_driver, session, host, port);

    async_task_executor.enqueue(dns_task);
    this->m_dns_task = ::std::move(dns_task);
    this->m_queue = ::std::move(queue);
    this->m_session = ::std::move(session);
  }

void
Easy_WS_Client::
close() noexcept
  {
    this->m_dns_task = nullptr;
    this->m_queue = nullptr;
    this->m_session = nullptr;
  }

const Socket_Address&
Easy_WS_Client::
local_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->local_address();
  }

const Socket_Address&
Easy_WS_Client::
remote_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->remote_address();
  }

bool
Easy_WS_Client::
ws_send(WebSocket_OpCode opcode, chars_proxy data)
  {
    if(!this->m_session)
      return false;

    return this->m_session->ws_send(opcode, data);
  }

bool
Easy_WS_Client::
ws_shut_down(uint16_t status, chars_proxy reason) noexcept
  {
    if(!this->m_session)
      return false;

    return this->m_session->ws_shut_down(status, reason);
  }

}  // namespace poseidon
