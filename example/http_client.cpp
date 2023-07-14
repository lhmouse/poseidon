// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_http_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/fiber/abstract_fiber.hpp"
#include "../poseidon/fiber/dns_future.hpp"
#include "../poseidon/static/async_task_executor.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_HTTP_Client my_client;
extern Easy_Timer my_timer;

void
event_callback(shptrR<HTTP_Client_Session> session, Abstract_Fiber& /*fiber*/, HTTP_Response_Headers&& resp, linear_buffer&& data)
  {
    POSEIDON_LOG_ERROR(("HTTP client received response from `$1`: $2 $3"),
        session->remote_address(), resp.status, resp.reason);

    for(const auto& pair : resp.headers)
      POSEIDON_LOG_ERROR(("  $1 --> $2"), pair.first, pair.second);

    POSEIDON_LOG_ERROR(("    payload ($1 bytes):\n$2"), data.size(), data);
  }

void
timer_callback(shptrR<Abstract_Timer> /*timer*/, Abstract_Fiber& fiber, steady_time /*now*/)
  {
    static const cow_string host = sref("www.example.org");
    static uint32_t state;

    if(my_client.session_opt() == nullptr)
      state = 0;
    else
      state ++;

    switch(state) {
      case 0: {
        auto dns = new_sh<DNS_Future>(host);
        async_task_executor.enqueue(dns);

        POSEIDON_LOG_ERROR(("DNS request: `$1`"), host);
        fiber.yield(dns);

        if(dns->result().addrs.empty()) {
          POSEIDON_LOG_ERROR(("DNS error: `$1` no address"), host);
          break;
        }

        auto& addr = dns->mut_result().addrs.mut(0);
        addr.set_port(80);
        POSEIDON_LOG_ERROR(("DNS result: `$1` = `$2`"), host, addr);

        my_client.connect(addr);
        POSEIDON_LOG_ERROR(("example HTTP client connecting: addr = $1"), addr);
        break;
      }

      case 1: {
        HTTP_Request_Headers req;
        req.uri = sref("/");
        req.headers.emplace_back(sref("Host"), host);
        req.headers.emplace_back(sref("Connection"), sref("keep-alive"));
        my_client.http_GET(::std::move(req));
        POSEIDON_LOG_ERROR(("example HTTP client: $1 $2"), req.method, req.uri);
        break;
      }

      case 2: {
        HTTP_Request_Headers req;
        req.uri = sref("/");
        req.headers.emplace_back(sref("Host"), host);
        my_client.http_POST(::std::move(req), "testdata");
        POSEIDON_LOG_ERROR(("example HTTP client: $1 $2"), req.method, req.uri);
        break;
      }

      case 3: {
        HTTP_Request_Headers req;
        req.uri = sref("/");
        req.headers.emplace_back(sref("Host"), host);
        my_client.http_DELETE(::std::move(req));
        POSEIDON_LOG_ERROR(("example HTTP client: $1 $2"), req.method, req.uri);
        break;
      }

      default:
        POSEIDON_LOG_ERROR(("example HTTP client shutting down"));
        my_client.close();
        break;
    }
  }

int
start_timer()
  {
    my_timer.start((seconds) 1, (seconds) 2);
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_HTTP_Client my_client(event_callback);
Easy_Timer my_timer(timer_callback);
int dummy = start_timer();

}  // namespace
