// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_wss_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static constexpr char connect_addr[] = "localhost:3807/some/caddr";
static Easy_WSS_Client my_client;
static Easy_Timer my_timer;

static void
my_client_callback(const shptr<WSS_Client_Session>& session,
                   Abstract_Fiber& fiber, Easy_WS_Event event,
                   linear_buffer&& data)
  {
    (void) fiber;

    switch(event)
      {
      case easy_ws_open:
        POSEIDON_LOG_WARN(("example WSS client established connection to `$1`: $2"),
                          session->remote_address(), data);
        break;

      case easy_ws_text:
        POSEIDON_LOG_WARN(("example WSS client received TEXT data: $1"), data);
        break;

      case easy_ws_binary:
        POSEIDON_LOG_WARN(("example WSS client received BINARY data: $1"), data);
        break;

      case easy_ws_pong:
        POSEIDON_LOG_WARN(("example WSS client received PONG data: $1"), data);
        break;

      case easy_ws_close:
        POSEIDON_LOG_WARN(("example WSS client closed connection: $1"), data);
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
      }
  }

static void
my_timer_callback(const shptr<Abstract_Timer>& timer,
                  Abstract_Fiber& fiber, steady_time now)
  {
    (void) timer;
    (void) fiber;
    (void) now;

    static uint32_t state;
    static shptr<WSS_Client_Session> my_client_session;

    if(my_client_session == nullptr)
      state = 0;
    else
      state ++;

    switch(state)
      {
      case 0:
        {
          my_client_session = my_client.connect(&connect_addr, my_client_callback);
          POSEIDON_LOG_WARN(("example WSS client connecting: $1"), connect_addr);
        }
        break;

      case 1:
        {
          const char data[] = "some text data";
          my_client_session->wss_send(websocket_TEXT, data);
          POSEIDON_LOG_DEBUG(("example WSS client sent TEXT frame: $1"), data);
        }
        break;

      case 2:
        {
          const char data[] = "some binary data";
          my_client_session->wss_send(websocket_BINARY, data);
          POSEIDON_LOG_DEBUG(("example WSS client sent BINARY frame: $1"), data);
        }
        break;

      case 3:
        {
          // HACKS; DO NOT PLAY WITH THESE AT HOME.
          WebSocket_Frame_Header header;
          header.masked = 1;
          header.masking_key = 0x87654321U;

          // fragment 1
          header.fin = 0;
          header.opcode = websocket_TEXT;
          char data1[] = "fragmented";
          header.payload_len = sizeof(data1) - 1;

          tinyfmt_ln fmt;
          header.encode(fmt);
          header.mask_payload(data1, sizeof(data1) - 1);
          fmt.putn(data1, sizeof(data1) - 1);
          my_client_session->ssl_send(fmt);

          // nested PING
          header.fin = 1;
          header.opcode = websocket_PING;
          char ping1[] = "ping within fragmented text message";
          header.payload_len = sizeof(ping1) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(ping1, sizeof(ping1) - 1);
          fmt.putn(ping1, sizeof(ping1) - 1);
          my_client_session->ssl_send(fmt);

          // fragment 2
          header.fin = 0;
          header.opcode = websocket_CONTINUATION;
          char data2[] = " text";
          header.payload_len = sizeof(data2) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data2, sizeof(data2) - 1);
          fmt.putn(data2, sizeof(data2) - 1);
          my_client_session->ssl_send(fmt);

          // fragment 3
          header.fin = 1;
          header.opcode = websocket_CONTINUATION;
          char data3[] = " data";
          header.payload_len = sizeof(data3) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data3, sizeof(data3) - 1);
          fmt.putn(data3, sizeof(data3) - 1);
          my_client_session->ssl_send(fmt);
        }
        break;

      case 5:
        {
          // HACKS; DO NOT PLAY WITH THESE AT HOME.
          WebSocket_Frame_Header header;
          header.masked = 1;
          header.masking_key = 0x87654321U;

          // fragment 1
          header.fin = 0;
          header.opcode = websocket_BINARY;
          char data1[] = "fragmented";
          header.payload_len = sizeof(data1) - 1;

          tinyfmt_ln fmt;
          header.encode(fmt);
          header.mask_payload(data1, sizeof(data1) - 1);
          fmt.putn(data1, sizeof(data1) - 1);
          my_client_session->ssl_send(fmt);

          // fragment 2
          header.fin = 0;
          header.opcode = websocket_CONTINUATION;
          char data2[] = " binary";
          header.payload_len = sizeof(data2) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data2, sizeof(data2) - 1);
          fmt.putn(data2, sizeof(data2) - 1);
          my_client_session->ssl_send(fmt);

          // nested PING
          header.fin = 1;
          header.opcode = websocket_PING;
          char ping1[] = "ping within fragmented binary message";
          header.payload_len = sizeof(ping1) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(ping1, sizeof(ping1) - 1);
          fmt.putn(ping1, sizeof(ping1) - 1);
          my_client_session->ssl_send(fmt);

          // fragment 3
          header.fin = 1;
          header.opcode = websocket_CONTINUATION;
          char data3[] = " data";
          header.payload_len = sizeof(data3) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data3, sizeof(data3) - 1);
          fmt.putn(data3, sizeof(data3) - 1);
          my_client_session->ssl_send(fmt);
        }
        break;

      default:
        POSEIDON_LOG_DEBUG(("example WSS client shutting down"));
        my_client_session->wss_shut_down(websocket_status_going_away, "bye");
        my_client_session.reset();
      }
  }

void
poseidon_module_main()
  {
    my_timer.start(1s, 2s, my_timer_callback);
  }
