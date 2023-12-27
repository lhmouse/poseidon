// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_https_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../socket/async_connect.hpp"
#include "../static/async_task_executor.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<HTTPS_Client_Session> wsession;
    cacheline_barrier xcb_1;

    // shared fields between threads
    struct Event
      {
        Easy_Socket_Event type;
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
    Easy_HTTPS_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    explicit
    Final_Fiber(const Easy_HTTPS_Client::thunk_type& thunk, const shptr<Event_Queue>& queue)
      :
        m_thunk(thunk), m_wqueue(queue)
      {
      }

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
            // Process a response.
            this->m_thunk(session, *this, event.type, move(event.resp), move(event.data));

            if(event.close_now)
              session->ssl_shut_down();
          }
          catch(exception& stdex) {
            // Shut the connection down asynchronously. Pending output data
            // are discarded, but the user-defined callback will still be called
            // for remaining input data, in case there is something useful.
            session->quick_close();

            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy HTTPS client: $1"),
                stdex);
          }
        }
      }
  };

struct Final_HTTPS_Client_Session final : HTTPS_Client_Session
  {
    Easy_HTTPS_Client::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;
    cow_string m_host_header;

    explicit
    Final_HTTPS_Client_Session(const Easy_HTTPS_Client::thunk_type& thunk, const shptr<Event_Queue>& queue,
          cow_stringR host_header)
      :
        m_thunk(thunk), m_wqueue(queue), m_host_header(host_header)
      {
      }

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
    do_on_https_response_finish(HTTP_Response_Headers&& resp, linear_buffer&& data, bool close_now) override
      {
        Event_Queue::Event event;
        event.type = easy_socket_msg_bin;
        event.resp = move(resp);
        event.data = move(data);
        event.close_now = close_now;
        this->do_push_event_common(move(event));
      }

    virtual
    void
    do_abstract_socket_on_closed() override
      {
        char sbuf[1024];
        int err_code = errno;
        const char* err_str = ::strerror_r(err_code, sbuf, sizeof(sbuf));

        Event_Queue::Event event;
        event.type = easy_socket_close;
        event.data.puts(err_str);
        this->do_push_event_common(move(event));
      }

    void
    fix_headers(HTTP_Request_Headers& req) const
      {
        // Erase bad headers.
        for(size_t hindex = 0;  hindex < req.headers.size();  hindex ++)
          if(ascii_ci_equal(req.headers.at(hindex).first, sref("Host")))
            req.headers.erase(hindex --);

        // Add required headers.
        req.headers.emplace_back(sref("Host"), this->m_host_header);
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_HTTPS_Client, Event_Queue);

Easy_HTTPS_Client::
~Easy_HTTPS_Client()
  {
  }

void
Easy_HTTPS_Client::
connect(chars_view addr)
  {
    // Parse the address string, which shall contain a host name and an optional
    // port to connect. If no port is specified, 443 is implied.
    Network_Reference caddr;
    caddr.port_num = 443;
    if(parse_network_reference(caddr, addr) != addr.n)
      POSEIDON_THROW(("Invalid connect address `$1`"), addr);

    // Disallow superfluous components.
    if(caddr.path.p != nullptr)
      POSEIDON_THROW(("URI paths shall not be specified in connect address `$1`"), addr);

    if(caddr.query.p != nullptr)
      POSEIDON_THROW(("URI queries shall not be specified in connect address `$1`"), addr);

    if(caddr.fragment.p != nullptr)
      POSEIDON_THROW(("URI fragments shall not be specified in connect address `$1`"), addr);

    // Initiate the connection.
    auto queue = new_sh<X_Event_Queue>();
    auto session = new_sh<Final_HTTPS_Client_Session>(this->m_thunk, queue,
          format_string("$1:$2", caddr.host, caddr.port_num));

    queue->wsession = session;
    auto dns_task = new_sh<Async_Connect>(network_driver, session,
          caddr.host.str(), caddr.port_num);

    async_task_executor.enqueue(dns_task);
    this->m_dns_task = move(dns_task);
    this->m_queue = move(queue);
    this->m_session = move(session);
  }

void
Easy_HTTPS_Client::
close() noexcept
  {
    this->m_dns_task = nullptr;
    this->m_queue = nullptr;
    this->m_session = nullptr;
  }

const Socket_Address&
Easy_HTTPS_Client::
local_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->local_address();
  }

const Socket_Address&
Easy_HTTPS_Client::
remote_address() const noexcept
  {
    if(!this->m_session)
      return ipv6_invalid;

    return this->m_session->remote_address();
  }

bool
Easy_HTTPS_Client::
https_GET(HTTP_Request_Headers&& req)
  {
    if(!this->m_session)
      return false;

    req.method = "GET";
    static_cast<Final_HTTPS_Client_Session*>(this->m_session.get())->fix_headers(req);
    return this->m_session->https_request(move(req), "");
  }

bool
Easy_HTTPS_Client::
https_POST(HTTP_Request_Headers&& req, chars_view data)
  {
    if(!this->m_session)
      return false;

    req.method = "POST";
    static_cast<Final_HTTPS_Client_Session*>(this->m_session.get())->fix_headers(req);
    return this->m_session->https_request(move(req), data);
  }

bool
Easy_HTTPS_Client::
https_PUT(HTTP_Request_Headers&& req, chars_view data)
  {
    if(!this->m_session)
      return false;

    req.method = "PUT";
    static_cast<Final_HTTPS_Client_Session*>(this->m_session.get())->fix_headers(req);
    return this->m_session->https_request(move(req), data);
  }

bool
Easy_HTTPS_Client::
https_DELETE(HTTP_Request_Headers&& req)
  {
    if(!this->m_session)
      return false;

    req.method = "DELETE";
    static_cast<Final_HTTPS_Client_Session*>(this->m_session.get())->fix_headers(req);
    return this->m_session->https_request(move(req), "");
  }

}  // namespace poseidon
