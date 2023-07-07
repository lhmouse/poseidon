// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_https_server.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_HTTPS_Server my_server;

void
event_callback(shptrR<HTTPS_Server_Session> session, Abstract_Fiber& /*fiber*/, HTTP_Request_Headers&& req, linear_buffer&& data)
  {
    POSEIDON_LOG_WARN(("HTTPS request --> $1 $2"), req.method, req.uri);
    for(const auto& r : req.headers)
      POSEIDON_LOG_WARN(("HTTPS header --> $1: $2"), r.first, r.second);

    HTTP_Response_Headers resp;
    resp.status = 200;
    resp.headers.emplace_back(sref("Date"), system_clock::now());
    resp.headers.emplace_back(sref("Content-Type"), sref("text/plain"));

    tinyfmt_ln fmt;
    fmt << "request payload length = " << data.size() << "\n";

    session->https_response(::std::move(resp), fmt.data(), fmt.size());
  }

int
start_server()
  {
    Socket_Address addr("[::]:3805");
    my_server.start(addr);
    POSEIDON_LOG_WARN(("example HTTPS server started: bind = $1"), my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_HTTPS_Server my_server(event_callback);
int dummy = start_server();

}  // namespace
