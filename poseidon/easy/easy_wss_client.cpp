// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "easy_wss_client.hpp"
#include "enums.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../socket/async_connect.hpp"
#include "../socket/enums.hpp"
#include "../static/async_task_executor.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<WSS_Client_Session> wsession;
    cacheline_barrier xcb_1;

    // fiber-private fields; no locking needed
    linear_buffer data_stream;
    cacheline_barrier xcb_2;

    // shared fields between threads
    struct Event
      {
        Easy_WS_Event type;
        linear_buffer data;
      };

    mutable plain_mutex mutex;
    deque<Event> events;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_WSS_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_Fiber(const Easy_WSS_Client::thunk_type& thunk, shptrR<Event_Queue> queue)
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
          auto event = move(queue->events.front());
          queue->events.pop_front();
          lock.unlock();

          try {
            // Process a message.
            this->m_thunk(session, *this, event.type, move(event.data));
          }
          catch(exception& stdex) {
            // Shut the connection down with a message.
            session->wss_shut_down(1015);

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy SSL client: $1"),
                stdex);
          }
        }
      }
  };

struct FInal_Client_Session final : WSS_Client_Session
  {
    Easy_WSS_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    FInal_Client_Session(const Easy_WSS_Client::thunk_type& thunk,
          shptrR<Event_Queue> queue, cow_stringR host, cow_stringR path,
          cow_stringR query)
      :
        SSL_Socket(network_driver), WSS_Client_Session(host, path, query),
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
            fiber_scheduler.launch(new_uni<Final_Fiber>(this->m_thunk, queue));
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
    do_on_wss_connected(cow_string&& caddr) override
      {
        Event_Queue::Event event;
        event.type = easy_ws_open;
        event.data.putn(caddr.data(), caddr.size());
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_on_wss_message_finish(WebSocket_OpCode opcode, linear_buffer&& data) override
      {
        Event_Queue::Event event;

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
        Event_Queue::Event event;
        event.type = easy_ws_close;

        tinyfmt_ln fmt;
        fmt << status << ": " << reason;
        event.data = fmt.extract_buffer();

        this->do_push_event_common(move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_WSS_Client, Event_Queue);

Easy_WSS_Client::
~Easy_WSS_Client()
  {
  }

void
Easy_WSS_Client::
connect(chars_view addr)
  {
    // Parse the address string, which shall contain a host name and an optional
    // port to connect, followed by an optional path and an optional query string.
    // If no port is specified, 443 is implied.
    Network_Reference caddr;
    caddr.port_num = 443;
    if(parse_network_reference(caddr, addr) != addr.n)
      POSEIDON_THROW(("Invalid connect address `$1`"), addr);

    // Disallow superfluous components.
    if(caddr.fragment.p != nullptr)
      POSEIDON_THROW(("URI fragments shall not be specified in connect address `$1`"), addr);

    // Initiate the connection.
    auto queue = new_sh<X_Event_Queue>();
    auto session = new_sh<FInal_Client_Session>(this->m_thunk, queue,
          format_string("$1:$2", caddr.host, caddr.port_num),
          caddr.path.str(), caddr.query.str());

    queue->wsession = session;
    auto dns_task = new_sh<Async_Connect>(network_driver, session,
          caddr.host.str(), caddr.port_num);

    async_task_executor.enqueue(dns_task);
    this->m_dns_task = move(dns_task);
    this->m_queue = move(queue);
    this->m_session = move(session);
  }

void
Easy_WSS_Client::
close() noexcept
  {
    this->m_dns_task = nullptr;
    this->m_queue = nullptr;
    this->m_session = nullptr;
  }

const Socket_Address&
Easy_WSS_Client::
local_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->local_address();
  }

const Socket_Address&
Easy_WSS_Client::
remote_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->remote_address();
  }

bool
Easy_WSS_Client::
wss_send(Easy_WS_Event opcode, chars_view data)
  {
    if(!this->m_session)
      return false;

    return this->m_session->wss_send(opcode, data);
  }

bool
Easy_WSS_Client::
wss_shut_down(uint16_t status, chars_view reason) noexcept
  {
    if(!this->m_session)
      return false;

    return this->m_session->wss_shut_down(status, reason);
  }

}  // namespace poseidon
