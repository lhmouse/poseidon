// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_ws_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static Easy_WS_Client my_client(
  // callback
  *[](shptrR<WS_Client_Session> session, Abstract_Fiber& fiber, Easy_WS_Event event,
      linear_buffer&& data)
  {
    (void) fiber;

    switch(event)
      {
      case easy_ws_open:
        POSEIDON_LOG_WARN(("example WS client established connection to `$1`: $2"),
                          session->remote_address(), data);
        break;

      case easy_ws_text:
        POSEIDON_LOG_WARN(("example WS client received TEXT data: $1"), data);
        break;

      case easy_ws_binary:
        POSEIDON_LOG_WARN(("example WS client received BINARY data: $1"), data);
        break;

      case easy_ws_pong:
        POSEIDON_LOG_WARN(("example WS client received PONG data: $1"), data);
        break;

      case easy_ws_close:
        POSEIDON_LOG_WARN(("example WS client closed connection: $1"), data);
        break;

      default:
        ASTERIA_TERMINATE(("shouldn't happen: event = $1"), event);
      }
  });

static Easy_Timer my_timer(
  // callback
  *[](shptrR<Abstract_Timer> timer, Abstract_Fiber& fiber, steady_time now)
  {
    (void) timer;
    (void) fiber;
    (void) now;

    static uint32_t state;
    static shptr<WS_Client_Session> my_client_session;

    if(my_client_session == nullptr)
      state = 0;
    else
      state ++;

    switch(state)
      {
      case 0:
        {
          cow_string addr = &"localhost:3806/some/caddr?arg1=foo&arg2=bar";
          my_client_session = my_client.connect(addr);
          POSEIDON_LOG_WARN(("example WS client connecting: addr = $1"), addr);
        }
        break;

      case 1:
        {
          const char data[] = "some text data";
          my_client_session->ws_send(easy_ws_text, data);
          POSEIDON_LOG_DEBUG(("example WS client sent TEXT frame: $1"), data);
        }
        break;

      case 2:
        {
          const char data[] = "some binary data";
          my_client_session->ws_send(easy_ws_binary, data);
          POSEIDON_LOG_DEBUG(("example WS client sent BINARY frame: $1"), data);
        }
        break;

      case 3:
        {
          // HACKS; DO NOT PLAY WITH THESE AT HOME.
          WebSocket_Frame_Header header;
          header.mask = 1;
          header.mask_key = 0x87654321U;

          // fragment 1
          header.fin = 0;
          header.opcode = 1;
          char data1[] = "fragmented";
          header.payload_len = sizeof(data1) - 1;

          tinyfmt_ln fmt;
          header.encode(fmt);
          header.mask_payload(data1, sizeof(data1) - 1);
          fmt.putn(data1, sizeof(data1) - 1);
          my_client_session->tcp_send(fmt);

          // nested PING
          header.fin = 1;
          header.opcode = 9;
          char ping1[] = "ping within fragmented text message";
          header.payload_len = sizeof(ping1) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(ping1, sizeof(ping1) - 1);
          fmt.putn(ping1, sizeof(ping1) - 1);
          my_client_session->tcp_send(fmt);

          // fragment 2
          header.fin = 0;
          header.opcode = 0;
          char data2[] = " text";
          header.payload_len = sizeof(data2) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data2, sizeof(data2) - 1);
          fmt.putn(data2, sizeof(data2) - 1);
          my_client_session->tcp_send(fmt);

          // fragment 3
          header.fin = 1;
          header.opcode = 0;
          char data3[] = " data";
          header.payload_len = sizeof(data3) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data3, sizeof(data3) - 1);
          fmt.putn(data3, sizeof(data3) - 1);
          my_client_session->tcp_send(fmt);
        }
        break;

      case 5:
        {
          // HACKS; DO NOT PLAY WITH THESE AT HOME.
          WebSocket_Frame_Header header;
          header.mask = 1;
          header.mask_key = 0x87654321U;

          // fragment 1
          header.fin = 0;
          header.opcode = 2;
          char data1[] = "fragmented";
          header.payload_len = sizeof(data1) - 1;

          tinyfmt_ln fmt;
          header.encode(fmt);
          header.mask_payload(data1, sizeof(data1) - 1);
          fmt.putn(data1, sizeof(data1) - 1);
          my_client_session->tcp_send(fmt);

          // fragment 2
          header.fin = 0;
          header.opcode = 0;
          char data2[] = " binary";
          header.payload_len = sizeof(data2) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data2, sizeof(data2) - 1);
          fmt.putn(data2, sizeof(data2) - 1);
          my_client_session->tcp_send(fmt);

          // nested PING
          header.fin = 1;
          header.opcode = 9;
          char ping1[] = "ping within fragmented binary message";
          header.payload_len = sizeof(ping1) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(ping1, sizeof(ping1) - 1);
          fmt.putn(ping1, sizeof(ping1) - 1);
          my_client_session->tcp_send(fmt);

          // fragment 3
          header.fin = 1;
          header.opcode = 0;
          char data3[] = " data";
          header.payload_len = sizeof(data3) - 1;

          fmt.clear_buffer();
          header.encode(fmt);
          header.mask_payload(data3, sizeof(data3) - 1);
          fmt.putn(data3, sizeof(data3) - 1);
          my_client_session->tcp_send(fmt);
        }
        break;

      default:
        POSEIDON_LOG_DEBUG(("example WS client shutting down"));
        my_client_session->ws_shut_down(3456, "bye");
        my_client_session.reset();
      }
  });

void
poseidon_module_main()
  {
    my_timer.start(1s, 2s);
  }
