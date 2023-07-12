// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_http_client.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_HTTP_Client my_client;

void
event_callback(shptrR<HTTP_Client_Session> session, Abstract_Fiber& /*fiber*/, HTTP_Response_Headers&& resp, linear_buffer&& data)
  {
    POSEIDON_LOG_ERROR(("HTTP client received response from `$1`: $2 $3"),
        session->remote_address(), resp.status, resp.reason);

    for(const auto& pair : resp.headers)
      POSEIDON_LOG_ERROR(("  $1 --> $2"), pair.first, pair.second);

    POSEIDON_LOG_ERROR(("    payload ($1 bytes):\n$2"), data.size(), data);
  }

int
start_client()
  {
    Socket_Address addr("93.184.216.34:80");  // www.example.org

    my_client.connect(addr);
    POSEIDON_LOG_FATAL(("example HTTP client started: local = $1"), my_client.local_address());

    HTTP_Request_Headers req;
    req.uri = sref("/");
    req.headers.emplace_back(sref("Host"), sref("www.example.org"));
    req.headers.emplace_back(sref("Connection"), sref("keep-alive"));
    my_client.http_GET(::std::move(req));

    req.uri = sref("/");
    req.headers.clear();
    req.headers.emplace_back(sref("Host"), sref("www.example.org"));
    my_client.http_POST(::std::move(req), "testdata");

    req.uri = sref("/");
    req.headers.emplace_back(sref("Host"), sref("www.example.org"));
    req.headers.emplace_back(sref("Connection"), sref("close"));
    my_client.http_DELETE(::std::move(req));

    POSEIDON_LOG_FATAL(("example HTTP client request sent"));
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_HTTP_Client my_client(event_callback);
int dummy = start_client();

}  // namespace
