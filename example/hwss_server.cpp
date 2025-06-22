// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_hwss_server.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static constexpr char bind_addr[] = "[::]:3807";
static Easy_HWSS_Server my_server;

static void
my_server_callback(const shptr<WSS_Server_Session>& session,
                   Abstract_Fiber& fiber, Easy_HWS_Event event,
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
        session->wss_send(ws_TEXT, data);
        break;

      case easy_hws_binary:
        POSEIDON_LOG_ERROR(("example HTTPS/WSS server received BINARY data: $1"), data);
        session->wss_send(ws_BINARY, data);
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

          HTTP_S_Headers resp;
          resp.status = http_status_ok;
          resp.headers.emplace_back(&"Content-Type", &"text/plain");

          if(event == easy_hws_head)
            session->https_response_headers_only(move(resp));
          else
            session->https_response(move(resp), "response from example HTTPS/WSS server\n");
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
