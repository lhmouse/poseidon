// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_hwss_server.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static Easy_HWSS_Server my_server(
  // callback
  *[](shptrR<WSS_Server_Session> session, Abstract_Fiber& fiber, Easy_HWS_Event event,
      linear_buffer&& data)
  {
    (void) fiber;

    switch(event)
      {
      case easy_hws_open:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server accepted connection from `$1`: $2"),
                           session->remote_address(), data);
        break;

      case easy_hws_text:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server received TEXT data: $1"), data);
        session->wss_send(easy_ws_text, data);
        break;

      case easy_hws_binary:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server received BINARY data: $1"), data);
        session->wss_send(easy_ws_binary, data);
        break;

      case easy_hws_pong:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server received PONG data: $1"), data);
        break;

      case easy_hws_close:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server closed connection: $1"), data);
        break;

      case easy_hws_get:
      case easy_hws_head:
        {
          POSEIDON_LOG_ERROR(("example HTTPS/WSS server received HTTP: $1"), data);

          HTTP_Response_Headers resp;
          resp.status = 200;
          resp.headers.emplace_back(&"Content-Type", &"text/plain");

          if(event == easy_hws_get)
            session->https_response(move(resp), "response from example HTTPS/WSS server\n");
          else
            session->https_response_headers_only(move(resp));
        }
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
      }
  });

void
poseidon_module_main()
  {
    my_server.start(&"[::]:3807");
    POSEIDON_LOG_ERROR(("example HTTPS/WSS server started: $1"), my_server.local_address());
  }