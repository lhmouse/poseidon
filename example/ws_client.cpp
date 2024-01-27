// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.hpp"
#include "../poseidon/easy/easy_ws_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/easy/enums.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_WS_Client my_client;
extern Easy_Timer my_timer;

void
event_callback(shptrR<WS_Client_Session> session, Abstract_Fiber& /*fiber*/,
               Easy_WS_Event event, linear_buffer&& data)
  {
    switch(event) {
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
  }

void
timer_callback(shptrR<Abstract_Timer> /*timer*/, Abstract_Fiber& /*fiber*/,
               steady_time /*now*/)
  {
    static uint32_t state;

    if(my_client.session_opt() == nullptr)
      state = 0;
    else
      state ++;

    switch(state) {
      case 0: {
        cow_string addr = sref("localhost:3806/some/caddr?arg1=foo&arg2=bar");
        my_client.connect(addr);
        POSEIDON_LOG_WARN(("example WS client connecting: addr = $1"), addr);
        break;
      }

      case 1: {
        const char data[] = "some text data";
        my_client.ws_send(easy_ws_text, data);
        POSEIDON_LOG_DEBUG(("example WS client sent TEXT frame: $1"), data);
        break;
      }

      case 2: {
        const char data[] = "some binary data";
        my_client.ws_send(easy_ws_binary, data);
        POSEIDON_LOG_DEBUG(("example WS client sent BINARY frame: $1"), data);
        break;
      }

      case 3: {
        // HACKS; DO NOT PLAY WITH THESE AT HOME.
        WebSocket_Frame_Header header;
        header.mask = 1;
        header.mask_key_u32 = 0x87654321;

        // fragment 1
        header.fin = 0;
        header.opcode = 1;
        char data1[] = "fragmented";
        header.payload_len = sizeof(data1) - 1;

        tinyfmt_ln fmt;
        header.encode(fmt);
        header.mask_payload(data1, sizeof(data1) - 1);
        fmt.putn(data1, sizeof(data1) - 1);
        my_client.session_opt()->tcp_send(fmt);

        // nested PING
        header.fin = 1;
        header.opcode = 9;
        char ping1[] = "ping within fragmented text message";
        header.payload_len = sizeof(ping1) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(ping1, sizeof(ping1) - 1);
        fmt.putn(ping1, sizeof(ping1) - 1);
        my_client.session_opt()->tcp_send(fmt);

        // fragment 2
        header.fin = 0;
        header.opcode = 0;
        char data2[] = " text";
        header.payload_len = sizeof(data2) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data2, sizeof(data2) - 1);
        fmt.putn(data2, sizeof(data2) - 1);
        my_client.session_opt()->tcp_send(fmt);

        // fragment 3
        header.fin = 1;
        header.opcode = 0;
        char data3[] = " data";
        header.payload_len = sizeof(data3) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data3, sizeof(data3) - 1);
        fmt.putn(data3, sizeof(data3) - 1);
        my_client.session_opt()->tcp_send(fmt);
        break;
      }

      case 5: {
        // HACKS; DO NOT PLAY WITH THESE AT HOME.
        WebSocket_Frame_Header header;
        header.mask = 1;
        header.mask_key_u32 = 0x87654321;

        // fragment 1
        header.fin = 0;
        header.opcode = 2;
        char data1[] = "fragmented";
        header.payload_len = sizeof(data1) - 1;

        tinyfmt_ln fmt;
        header.encode(fmt);
        header.mask_payload(data1, sizeof(data1) - 1);
        fmt.putn(data1, sizeof(data1) - 1);
        my_client.session_opt()->tcp_send(fmt);

        // fragment 2
        header.fin = 0;
        header.opcode = 0;
        char data2[] = " binary";
        header.payload_len = sizeof(data2) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data2, sizeof(data2) - 1);
        fmt.putn(data2, sizeof(data2) - 1);
        my_client.session_opt()->tcp_send(fmt);

        // nested PING
        header.fin = 1;
        header.opcode = 9;
        char ping1[] = "ping within fragmented binary message";
        header.payload_len = sizeof(ping1) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(ping1, sizeof(ping1) - 1);
        fmt.putn(ping1, sizeof(ping1) - 1);
        my_client.session_opt()->tcp_send(fmt);

        // fragment 3
        header.fin = 1;
        header.opcode = 0;
        char data3[] = " data";
        header.payload_len = sizeof(data3) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data3, sizeof(data3) - 1);
        fmt.putn(data3, sizeof(data3) - 1);
        my_client.session_opt()->tcp_send(fmt);
        break;
      }

      default:
        POSEIDON_LOG_DEBUG(("example WS client shutting down"));
        my_client.ws_shut_down(3456, "bye");
        my_client.close();
        break;
    }
  }

int
start_timer()
  {
    my_timer.start(1s, 2s);
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_WS_Client my_client(event_callback);
Easy_Timer my_timer(timer_callback);
int dummy = start_timer();

}  // namespace
