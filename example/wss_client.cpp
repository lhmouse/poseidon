// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_wss_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_WSS_Client my_client;
extern Easy_Timer my_timer;

void
event_callback(shptrR<WSS_Client_Session> session, Abstract_Fiber& /*fiber*/, WebSocket_Event event, linear_buffer&& data)
  {
    switch(event) {
      case websocket_open:
        POSEIDON_LOG_WARN(("example WSS client connected to server: $1"), session->remote_address());
        break;

      case websocket_text:
        POSEIDON_LOG_WARN(("example WSS client received TEXT data: $1"), data);
        break;

      case websocket_binary:
        POSEIDON_LOG_WARN(("example WSS client received BINARY data: $1"), data);
        break;

      case websocket_pong:
        POSEIDON_LOG_WARN(("example WSS client received PONG data: $1"), data);
        break;

      case websocket_closed:
        POSEIDON_LOG_WARN(("example WSS client shut down connection: $1"), data);
        break;
    }
  }

void
timer_callback(shptrR<Abstract_Timer> /*timer*/, Abstract_Fiber& /*fiber*/, steady_time /*now*/)
  {
    static uint32_t state;

    if(my_client.session_opt() == nullptr)
      state = 0;
    else
      state ++;

    switch(state) {
      case 0: {
        Socket_Address addr("127.0.0.1:3807");
        my_client.connect(addr, sref("/some/uri"));
        POSEIDON_LOG_WARN(("example WSS client connecting: addr = $1"), addr);
        break;
      }

      case 1: {
        const char data[] = "some text data";
        my_client.wss_send_text(data);
        POSEIDON_LOG_DEBUG(("example WSS client sent TEXT frame: $1"), data);
        break;
      }

      case 2: {
        const char data[] = "some binary data";
        my_client.wss_send_binary(data);
        POSEIDON_LOG_DEBUG(("example WSS client sent BINARY frame: $1"), data);
        break;
      }

      case 3: {
        const char data[] = "some ping data";
        my_client.wss_ping(data);
        POSEIDON_LOG_DEBUG(("example WSS client sent PING frame: $1"), data);
        break;
      }

      case 4: {
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
        my_client.session_opt()->ssl_send(fmt);

        // nested PING
        header.fin = 1;
        header.opcode = 9;
        char ping1[] = "PING";
        header.payload_len = sizeof(ping1) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(ping1, sizeof(ping1) - 1);
        fmt.putn(ping1, sizeof(ping1) - 1);
        my_client.session_opt()->ssl_send(fmt);

        // fragment 2
        header.fin = 0;
        header.opcode = 0;
        char data2[] = " text";
        header.payload_len = sizeof(data2) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data2, sizeof(data2) - 1);
        fmt.putn(data2, sizeof(data2) - 1);
        my_client.session_opt()->ssl_send(fmt);

        // fragment 3
        header.fin = 1;
        header.opcode = 0;
        char data3[] = " data";
        header.payload_len = sizeof(data3) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data3, sizeof(data3) - 1);
        fmt.putn(data3, sizeof(data3) - 1);
        my_client.session_opt()->ssl_send(fmt);
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
        my_client.session_opt()->ssl_send(fmt);

        // fragment 2
        header.fin = 0;
        header.opcode = 0;
        char data2[] = " binary";
        header.payload_len = sizeof(data2) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data2, sizeof(data2) - 1);
        fmt.putn(data2, sizeof(data2) - 1);
        my_client.session_opt()->ssl_send(fmt);

        // nested PING
        header.fin = 1;
        header.opcode = 9;
        char ping1[] = "PING";
        header.payload_len = sizeof(ping1) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(ping1, sizeof(ping1) - 1);
        fmt.putn(ping1, sizeof(ping1) - 1);
        my_client.session_opt()->ssl_send(fmt);

        // fragment 3
        header.fin = 1;
        header.opcode = 0;
        char data3[] = " data";
        header.payload_len = sizeof(data3) - 1;

        fmt.clear_buffer();
        header.encode(fmt);
        header.mask_payload(data3, sizeof(data3) - 1);
        fmt.putn(data3, sizeof(data3) - 1);
        my_client.session_opt()->ssl_send(fmt);
        break;
      }

      default:
        POSEIDON_LOG_DEBUG(("example WSS client shutting down"));
        my_client.wss_shut_down(3456, "bye");
        my_client.close();
        break;
    }
  }

int
start_timer()
  {
    my_timer.start((seconds) 1, (seconds) 2);
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_WSS_Client my_client(event_callback);
Easy_Timer my_timer(timer_callback);
int dummy = start_timer();

}  // namespace
