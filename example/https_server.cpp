// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_https_server.hpp"
#include "../poseidon/easy/enums.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_HTTPS_Server my_server;

void
event_callback(shptrR<HTTPS_Server_Session> session, Abstract_Fiber& /*fiber*/,
               Easy_HTTP_Event event,HTTP_Request_Headers&& req, linear_buffer&& data)
  {
    switch(event) {
      case easy_http_open:
        POSEIDON_LOG_WARN(("example HTTPS server accepted connection: $1"),
                           session->remote_address());
        break;

      case easy_http_message: {
        POSEIDON_LOG_WARN(("HTTPS request --> $1 $2: $3"),
                          req.method, req.uri_path, req.uri_query);
        for(const auto& r : req.headers)
          POSEIDON_LOG_WARN(("HTTPS header --> $1: $2"), r.first, r.second);

        // send a response
        HTTP_Response_Headers resp;
        resp.status = 200;
        resp.headers.emplace_back(sref("Date"), system_clock::now());
        resp.headers.emplace_back(sref("Content-Type"), sref("text/plain"));

        tinyfmt_ln fmt;
        fmt << "request payload length = " << data.size() << "\n";

        session->https_response(move(resp), fmt);
        break;
      }

      case easy_http_close:
        POSEIDON_LOG_WARN(("example HTTPS server shut down connection: $1"), data);
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
    }
  }

int
start_server()
  {
    my_server.start(sref("[::]:3805"));
    POSEIDON_LOG_WARN(("example HTTPS server started: bind = $1"),
                      my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_HTTPS_Server my_server(event_callback);
int dummy = start_server();

}  // namespace
