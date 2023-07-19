// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_ws_server.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_WS_Server my_server;

void
event_callback(shptrR<WS_Server_Session> session, Abstract_Fiber& /*fiber*/, Easy_Socket_Event event, linear_buffer&& data)
  {
    switch(event) {
      case easy_socket_open:
        POSEIDON_LOG_ERROR(("example WS server accepted connection from `$1`: $2"), session->remote_address(), data);
        break;

      case easy_socket_msg_text:
        POSEIDON_LOG_ERROR(("example WS server received TEXT data: $1"), data);
        session->ws_send(websocket_text, data);
        break;

      case easy_socket_msg_bin:
        POSEIDON_LOG_ERROR(("example WS server received BINARY data: $1"), data);
        session->ws_send(websocket_bin, data);
        break;

      case easy_socket_pong:
        POSEIDON_LOG_ERROR(("example WS server received PONG data: $1"), data);
        break;

      case easy_socket_close:
        POSEIDON_LOG_ERROR(("example WS server shut down connection: $1"), data);
        break;

      case easy_socket_stream:
      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
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
