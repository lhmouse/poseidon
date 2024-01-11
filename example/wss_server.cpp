// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_wss_server.hpp"
#include "../poseidon/easy/enums.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_WSS_Server my_server;

void
event_callback(shptrR<WSS_Server_Session> session, Abstract_Fiber& /*fiber*/,
               Easy_WS_Event event, linear_buffer&& data)
  {
    switch(event) {
      case easy_ws_open:
        POSEIDON_LOG_ERROR(("example WSS server accepted connection from `$1`: $2"),
                           session->remote_address(), data);
        break;

      case easy_ws_text:
        POSEIDON_LOG_ERROR(("example WSS server received TEXT data: $1"), data);
        session->wss_send(easy_ws_text, data);
        break;

      case easy_ws_binary:
        POSEIDON_LOG_ERROR(("example WSS server received BINARY data: $1"), data);
        session->wss_send(easy_ws_binary, data);
        break;

      case easy_ws_pong:
        POSEIDON_LOG_ERROR(("example WSS server received PONG data: $1"), data);
        break;

      case easy_ws_close:
        POSEIDON_LOG_ERROR(("example WSS server shut down connection: $1"), data);
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
    }
  }

int
start_server()
  {
    my_server.start(sref("[::]:3807"));
    POSEIDON_LOG_ERROR(("example WSS server started: bind = $1"),
                       my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_WSS_Server my_server(event_callback);
int dummy = start_server();

}  // namespace
