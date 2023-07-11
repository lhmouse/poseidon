// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_ws_server.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_WS_Server my_server;

void
event_callback(shptrR<WS_Server_Session> session, Abstract_Fiber& /*fiber*/, WebSocket_Event event, linear_buffer&& data)
  {
    switch(event) {
      case websocket_open:
        POSEIDON_LOG_ERROR(("example WS server accepted connection: $1"), session->remote_address());
        break;

      case websocket_text:
        POSEIDON_LOG_ERROR(("example WS server received TEXT data: $1"), data);
        session->ws_send_text(data);
        break;

      case websocket_binary:
        POSEIDON_LOG_ERROR(("example WS server received BINARY data: $1"), data);
        session->ws_send_binary(data);
        break;

      case websocket_pong:
        POSEIDON_LOG_ERROR(("example WS server received PONG data: $1"), data);
        break;

      case websocket_closed:
        POSEIDON_LOG_ERROR(("example WS server shut down connection: $1"), data);
        break;
    }
  }

int
start_server()
  {
    Socket_Address addr("[::]:3806");
    my_server.start(addr);
    POSEIDON_LOG_ERROR(("example WS server started: bind = $1"), my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_WS_Server my_server(event_callback);
int dummy = start_server();

}  // namespace
