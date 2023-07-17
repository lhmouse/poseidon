// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_http_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<HTTP_Client_Session> wsession;
    cacheline_barrier xcb_1;

    // shared fields between threads
    struct Event
      {
        HTTP_Response_Headers resp;
        linear_buffer data;
        bool close_now = false;
      };

    mutable plain_mutex mutex;
    deque<Event> events;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_HTTP_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_Fiber(const Easy_HTTP_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
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
            // Process a response.
            this->m_thunk(session, *this, ::std::move(event.resp), ::std::move(event.data));

            if(event.close_now)
              session->tcp_shut_down();
          }
          catch(exception& stdex) {
            // Shut the connection down asynchronously. Pending output data
            // are discarded, but the user-defined callback will still be called
            // for remaining input data, in case there is something useful.
            session->quick_close();

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy HTTP client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_HTTP_Client_Session final : HTTP_Client_Session
  {
    Easy_HTTP_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_HTTP_Client_Session(const Easy_HTTP_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
      : m_thunk(thunk), m_wqueue(queue)  { }

    virtual
    void
    do_on_http_response_finish(HTTP_Response_Headers&& resp, linear_buffer&& data, bool close_now) override
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

        Event_Queue::Event event;
        event.resp = ::std::move(resp);
        event.data = ::std::move(data);
        event.close_now = close_now;
        queue->events.push_back(::std::move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_HTTP_Client, Event_Queue);

Easy_HTTP_Client::
~Easy_HTTP_Client()
  {
  }

void
Easy_HTTP_Client::
connect(const Socket_Address& addr)
  {
    auto queue = new_sh<X_Event_Queue>();
    auto session = new_sh<Final_HTTP_Client_Session>(this->m_thunk, queue);
    queue->wsession = session;
    session->connect(addr);

    network_driver.insert(session);
    this->m_queue = ::std::move(queue);
    this->m_session = ::std::move(session);
  }

void
Easy_HTTP_Client::
close() noexcept
  {
    this->m_queue = nullptr;
    this->m_session = nullptr;
  }

const Socket_Address&
Easy_HTTP_Client::
local_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->local_address();
  }

const Socket_Address&
Easy_HTTP_Client::
remote_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->remote_address();
  }

bool
Easy_HTTP_Client::
http_GET(HTTP_Request_Headers&& req)
  {
    if(!this->m_session)
      return false;

    req.method = sref("GET");
    return this->m_session->http_request(::std::move(req), "");
  }

bool
Easy_HTTP_Client::
http_POST(HTTP_Request_Headers&& req, chars_proxy data)
  {
    if(!this->m_session)
      return false;

    req.method = sref("POST");
    return this->m_session->http_request(::std::move(req), data);
  }

bool
Easy_HTTP_Client::
http_PUT(HTTP_Request_Headers&& req, chars_proxy data)
  {
    if(!this->m_session)
      return false;

    req.method = sref("PUT");
    return this->m_session->http_request(::std::move(req), data);
  }

bool
Easy_HTTP_Client::
http_DELETE(HTTP_Request_Headers&& req)
  {
    if(!this->m_session)
      return false;

    req.method = sref("DELETE");
    return this->m_session->http_request(::std::move(req), "");
  }

}  // namespace poseidon
