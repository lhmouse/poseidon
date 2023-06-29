// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_https_client.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_HTTPS_Client my_client;

void
event_callback(shptrR<HTTPS_Client_Session> session, Abstract_Fiber& /*fiber*/, HTTP_Response_Headers&& resp, linear_buffer&& data)
  {
    POSEIDON_LOG_WARN(("HTTP client received response from `$1`: $2 $3"),
        session->remote_address(), resp.status, resp.reason);

    for(const auto& pair : resp.headers)
      POSEIDON_LOG_WARN(("  $1 --> $2"), pair.first, pair.second);

    POSEIDON_LOG_WARN(("    payload ($1 bytes):\n$2"), data.size(), data);
  }

int
start_client()
  {
    Socket_Address addr("93.184.216.34:443");  // www.example.org

    my_client.open(addr);
    POSEIDON_LOG_FATAL(("example HTTPS client started: local = $1"), my_client.local_address());

    HTTP_Request_Headers req;
    req.uri = sref("/");
    req.headers.emplace_back(sref("Host"), sref("www.example.org"));
    req.headers.emplace_back(sref("Connection"), sref("keep-alive"));
    my_client.https_GET(::std::move(req));

    req.uri = sref("/");
    req.headers.clear();
    req.headers.emplace_back(sref("Host"), sref("www.example.org"));
    my_client.https_POST(::std::move(req), "testdata", 8);

    req.uri = sref("/");
    req.headers.emplace_back(sref("Host"), sref("www.example.org"));
    req.headers.emplace_back(sref("Connection"), sref("close"));
    my_client.https_DELETE(::std::move(req));

    POSEIDON_LOG_FATAL(("example HTTPS client request sent"));
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_HTTPS_Client my_client(event_callback);
int dummy = start_client();

}  // namespace
