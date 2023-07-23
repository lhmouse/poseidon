// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_https_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_HTTPS_Client my_client;
extern Easy_Timer my_timer;

void
event_callback(shptrR<HTTPS_Client_Session> session, Abstract_Fiber& /*fiber*/, Easy_Socket_Event event, HTTP_Response_Headers&& resp, linear_buffer&& data)
  {
    switch(event) {
      case easy_socket_msg_bin: {
        POSEIDON_LOG_WARN(("HTTPS client received response from `$1`: $2 $3"),
            session->remote_address(), resp.status, resp.reason);

        for(const auto& pair : resp.headers)
          POSEIDON_LOG_WARN(("  $1 --> $2"), pair.first, pair.second);

        POSEIDON_LOG_WARN(("    payload ($1 bytes):\n$2"), data.size(), data);
        break;
      }

      case easy_socket_close:
        POSEIDON_LOG_WARN(("example HTTPS client shut down connection: $1"), data);
        break;

      case easy_socket_open:
      case easy_socket_stream:
      case easy_socket_msg_text:
      case easy_socket_pong:
      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
    }
  }

void
timer_callback(shptrR<Abstract_Timer> /*timer*/, Abstract_Fiber& /*fiber*/, steady_time /*now*/)
  {
    static uint32_t state;

    if(my_client.session_opt() == nullptr)
      state = 0;
    else
      state ++;

    switch(state) {
      case 0: {
        cow_string addr = sref("https://www.example.org");
        my_client.connect(addr);
        POSEIDON_LOG_WARN(("example HTTPS client connecting: addr = $1"), addr);
        break;
      }

      case 1: {
        HTTP_Request_Headers req;
        req.uri_path = sref("/");
        req.headers.emplace_back(sref("Connection"), sref("keep-alive"));
        my_client.https_GET(::std::move(req));
        POSEIDON_LOG_WARN(("example HTTPS client: $1 $2"), req.method, req.uri_path);
        break;
      }

      case 2: {
        HTTP_Request_Headers req;
        req.uri_path = sref("/");
        my_client.https_POST(::std::move(req), "testdata");
        POSEIDON_LOG_WARN(("example HTTPS client: $1 $2"), req.method, req.uri_path);
        break;
      }

      case 3: {
        HTTP_Request_Headers req;
        req.uri_path = sref("/");
        my_client.https_DELETE(::std::move(req));
        POSEIDON_LOG_WARN(("example HTTPS client: $1 $2"), req.method, req.uri_path);
        break;
      }

      default:
        POSEIDON_LOG_WARN(("example HTTPS client shutting down"));
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
Easy_HTTPS_Client my_client(event_callback);
Easy_Timer my_timer(timer_callback);
int dummy = start_timer();

}  // namespace
