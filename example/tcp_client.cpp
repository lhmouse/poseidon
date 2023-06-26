// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_tcp_client.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_TCP_Client my_client;

void
event_callback(shptrR<TCP_Socket> socket, Abstract_Fiber& /*fiber*/, Connection_Event event, linear_buffer& data, int code)
  {
    switch(event) {
      case connection_event_open:
        static constexpr char req[] = "GET / HTTP/1.1\r\nConnection: close\r\nHost: www.example.org\r\n\r\n";
        socket->tcp_send(req, ::strlen(req));
        POSEIDON_LOG_FATAL(("example TCP client sent data to `$1`:\n\n$2"), socket->remote_address(), req);
        break;

      case connection_event_stream:
        POSEIDON_LOG_WARN(("example TCP client received data from `$1` (eof = $2):\n\n$3"), socket->remote_address(), code, data);
        data.clear();
        break;

      case connection_event_closed:
        POSEIDON_LOG_FATAL(("example TCP client shut down connection `$1` (errno = $2): $3"), socket->remote_address(), code, data);
        break;
    }
  }

int
start_client()
  {
    Socket_Address addr("93.184.216.34:80");  // www.example.org

    my_client.open(addr);
    POSEIDON_LOG_FATAL(("example TCP client started: local = $1"), my_client.local_address());
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_TCP_Client my_client(event_callback);
int dummy = start_client();

}  // namespace
