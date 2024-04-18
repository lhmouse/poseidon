// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_ws_server.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static Easy_WS_Server my_server(
  // callback
  *[](shptrR<WS_Server_Session> session, Abstract_Fiber& fiber, Easy_WS_Event event,
      linear_buffer&& data)
  {
    (void) fiber;

    switch(event)
      {
      case easy_ws_open:
        POSEIDON_LOG_ERROR(("example WS server accepted connection from `$1`: $2"),
                           session->remote_address(), data);
        break;

      case easy_ws_text:
        POSEIDON_LOG_ERROR(("example WS server received TEXT data: $1"), data);
        session->ws_send(websocket_text, data);
        break;

      case easy_ws_binary:
        POSEIDON_LOG_ERROR(("example WS server received BINARY data: $1"), data);
        session->ws_send(websocket_binary, data);
        break;

      case easy_ws_pong:
        POSEIDON_LOG_ERROR(("example WS server received PONG data: $1"), data);
        break;

      case easy_ws_close:
        POSEIDON_LOG_ERROR(("example WS server closed connection: $1"), data);
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
      }
  });

void
poseidon_module_main()
  {
    static constexpr char bind_addr[] = "[::]:3806";
    my_server.start(&bind_addr);
    POSEIDON_LOG_ERROR(("example WS server started: $1"), bind_addr);
  }
