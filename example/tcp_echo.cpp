// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_tcp_server.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_TCP_Server my_server;

void
event_callback(shptrR<TCP_Socket> socket, Abstract_Fiber& /*fiber*/, Connection_Event event, linear_buffer& data)
  {
    Socket_Address addr = socket->remote_address();
    string str(data.data(), data.size());
    data.clear();

    switch(event) {
      case connection_event_null:
        break;

      case connection_event_open:
        POSEIDON_LOG_FATAL(("example TCP server accepted connection from `$1`"), addr);
        break;

      case connection_event_stream:
        POSEIDON_LOG_WARN(("example TCP server received data from `$1`: $2"), addr, str);
        socket->tcp_send(str.data(), str.size());
        break;

      case connection_event_closed:
        POSEIDON_LOG_FATAL(("example TCP server shut down connection `$1`: $2"), addr, str);
        break;
    }
  }

int
start_server()
  {
    Socket_Address addr("[::]:3802");
    my_server.start(addr);
    POSEIDON_LOG_ERROR(("example TCP server started: bind = $1"), my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_TCP_Server my_server(event_callback);
int dummy = start_server();

}  // namespace
