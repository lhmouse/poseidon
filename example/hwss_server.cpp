// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../poseidon/src/xprecompiled.hpp"
#include "../poseidon/easy/easy_hwss_server.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static constexpr char bind_addr[] = "[::]:3807";
static Easy_HWSS_Server my_server;

static void
my_server_callback(const shptr<WSS_Server_Session>& session,
      Abstract_Fiber& fiber, Easy_HWS_Event event, linear_buffer&& data)
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
        session->wss_send(ws_opcode_TEXT, data);
        break;

      case easy_hws_binary:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server received BINARY data: $1"), data);
        session->wss_send(ws_opcode_BINARY, data);
        break;

      case easy_hws_pong:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server received PONG data: $1"), data);
        break;

      case easy_hws_close:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server closed connection: $1"), data);
        break;

      case easy_hws_get:
        {
          POSEIDON_LOG_ERROR(("example HTTPS/WSS server received HTTP GET: $1"), data);

          HTTP_Response_Headers resp;
          resp.status = http_status_ok;
          resp.headers.emplace_back(&"Content-Type", &"text/plain");
          session->https_response(http_method_GET, resp, "example HTTPS/WSS server GET\n");
        }
        break;

      case easy_hws_head:
        {
          POSEIDON_LOG_ERROR(("example HTTPS/WSS server received HTTP HEAD: $1"), data);

          HTTP_Response_Headers resp;
          resp.status = http_status_ok;
          resp.headers.emplace_back(&"Content-Type", &"text/plain");
          session->https_response(http_method_HEAD, resp, "example HTTPS/WSS server HEAD\n");
        }
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
      }
  }

void
poseidon_module_main()
  {
    my_server.start(&bind_addr, my_server_callback);
    POSEIDON_LOG_ERROR(("example HTTPS/WSS server started: $1"), bind_addr);
  }
