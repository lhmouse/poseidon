// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_ssl_client.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_SSL_Client my_client;

void
event_callback(Abstract_Fiber& /*fiber*/, shptrR<SSL_Socket> socket, Connection_Event event, linear_buffer& data)
  {
    Socket_Address addr = socket->remote_address();
    string str(data.data(), data.size());
    data.clear();
    static constexpr char req[] = "GET / HTTP/1.1\r\nConnection: close\r\nHost: www.example.org\r\n\r\n";

    switch(event) {
      case connection_event_null:
        break;

      case connection_event_open:
        socket->ssl_send(req, ::strlen(req));
        POSEIDON_LOG_FATAL(("example SSL client sent data to `$1`:\n\n$2"), addr, req);
        break;

      case connection_event_stream:
        POSEIDON_LOG_WARN(("example SSL client received data from `$1`:\n\n$2"), addr, str);
        break;

      case connection_event_closed:
        POSEIDON_LOG_FATAL(("example SSL client shut down connection `$1`: $2"), addr, str);
        break;
    }
  }

int
start_client()
  {
    Socket_Address addr("93.184.216.34:443");  // www.example.org

    my_client.start(addr);
    POSEIDON_LOG_FATAL(("example SSL client started: local = $1"), my_client.local_address());
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_SSL_Client my_client(event_callback);
int dummy = start_client();

}  // namespace
