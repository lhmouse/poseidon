// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_tcp_client.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_TCP_Client my_client;

void
event_callback(shared_ptrR<TCP_Socket> socket, Connection_Event event, linear_buffer& data)
  {
    Socket_Address addr = socket->remote_address();
    cow_string str(data.data(), data.size());
    data.clear();
    static constexpr char req[] = "GET / HTTP/1.1\r\nConnection: close\r\nHost: www.example.org\r\n\r\n";

    switch((uint32_t) event) {
      case connection_event_open:
        socket->tcp_send(req, ::strlen(req));
        POSEIDON_LOG_FATAL(("example TCP client sent data to `$1`:\n\n$2"), addr, req);
        break;

      case connection_event_stream:
        POSEIDON_LOG_WARN(("example TCP client received data from `$1`:\n\n$2"), addr, str);
        break;

      case connection_event_closed:
        POSEIDON_LOG_FATAL(("example TCP client shut down connection `$1`: $2"), addr, str);
        break;
    }
  }

int
start_client()
  {
    Socket_Address addr("93.184.216.34:80");  // www.example.org

    my_client.start(addr);
    POSEIDON_LOG_FATAL(("example TCP client started: local = $1"), my_client.local_address());
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_TCP_Client my_client(event_callback);
int dummy = start_client();

}  // namespace
