// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_wss_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
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
        WebSocket_Event type;
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
    Final_Fiber(const Easy_WSS_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
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
            session->wss_shut_down(1015);

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy SSL client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_WSS_Client_Session final : WSS_Client_Session
  {
    Easy_WSS_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;
    cow_string m_uri;

    explicit
    Final_WSS_Client_Session(cow_stringR uri, const Easy_WSS_Client::thunk_type& thunk,
          const shptr<Event_Queue>& queue)
      : WSS_Client_Session(uri), m_thunk(thunk), m_wqueue(queue), m_uri(uri)
      { }

    void
    do_push_event_common(WebSocket_Event type, linear_buffer&& data) const
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

        auto& event = queue->events.emplace_back();
        event.type = type;
        event.data = ::std::move(data);
      }

    virtual
    void
    do_on_wss_connected(cow_string&& uri) override
      {
        linear_buffer data;
        data.putn(uri.data(), uri.size());
        this->do_push_event_common(websocket_open, ::std::move(data));
      }

    virtual
    void
    do_on_wss_text(linear_buffer&& data) override
      {
        this->do_push_event_common(websocket_text, ::std::move(data));
      }

    virtual
    void
    do_on_wss_binary(linear_buffer&& data) override
      {
        this->do_push_event_common(websocket_binary, ::std::move(data));
      }

    virtual
    void
    do_on_wss_pong(linear_buffer&& data) override
      {
        this->do_push_event_common(websocket_pong, ::std::move(data));
      }

    virtual
    void
    do_on_wss_close(uint16_t status, chars_proxy reason) override
      {
        tinyfmt_ln fmt;
        fmt << status << ": " << reason;
        this->do_push_event_common(websocket_closed, fmt.extract_buffer());
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
connect(const Socket_Address& addr, cow_stringR uri)
  {
    auto queue = new_sh<X_Event_Queue>();
    auto session = new_sh<Final_WSS_Client_Session>(uri, this->m_thunk, queue);
    queue->wsession = session;
    session->connect(addr);

    network_driver.insert(session);
    this->m_queue = ::std::move(queue);
    this->m_session = ::std::move(session);
  }

void
Easy_WSS_Client::
close() noexcept
  {
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
wss_send_text(chars_proxy data)
  {
    if(!this->m_session)
      return false;

    return this->m_session->wss_send_text(data);
  }

bool
Easy_WSS_Client::
wss_send_binary(chars_proxy data)
  {
    if(!this->m_session)
      return false;

    return this->m_session->wss_send_binary(data);
  }

bool
Easy_WSS_Client::
wss_ping(chars_proxy data)
  {
    if(!this->m_session)
      return false;

    return this->m_session->wss_ping(data);
  }

bool
Easy_WSS_Client::
wss_shut_down(uint16_t status, chars_proxy reason) noexcept
  {
    if(!this->m_session)
      return false;

    return this->m_session->wss_shut_down(status, reason);
  }

}  // namespace poseidon
