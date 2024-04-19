// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/websocket_frame_header.hpp"
using namespace ::poseidon;

int
main()
  {
    // https://datatracker.ietf.org/doc/html/rfc6455#section-5.7
    tinyfmt_str fmt;
    WebSocket_Frame_Header header;

    // A single-frame unmasked text message
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = websocket_TEXT;
    header.payload_len = 5;
    header.encode(fmt);
    fmt.putn("Hello", 5);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x81\x05\x48\x65\x6c\x6c\x6f", 7));

    // A single-frame masked text message
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = websocket_TEXT;
    header.mask = 1;
    header.mask_key = ROCKET_HTOBE32(0x37fa213d);
    header.payload_len = 5;
    header.encode(fmt);

    char xhello1[] = "Hello";
    header.mask_payload(xhello1, 5);
    fmt.putn(xhello1, 5);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x81\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58", 11));

    // A fragmented unmasked text message
    // 1
    fmt.clear_string();
    header.clear();

    header.opcode = websocket_TEXT;
    header.payload_len = 3;
    header.encode(fmt);
    fmt.putn("Hel", 3);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x01\x03\x48\x65\x6c", 5));

    // 2
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.payload_len = 2;
    header.encode(fmt);
    fmt.putn("lo", 2);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x80\x02\x6c\x6f", 4));

    // Unmasked Ping request and masked Ping response
    // 1
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = websocket_PING;
    header.payload_len = 5;
    header.encode(fmt);
    fmt.putn("Hello", 5);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x89\x05\x48\x65\x6c\x6c\x6f", 7));

    // 2
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = websocket_PONG;
    header.mask = 1;
    header.mask_key = ROCKET_HTOBE32(0x37fa213d);
    header.payload_len = 5;
    header.encode(fmt);

    char xhello2[] = "Hello";
    header.mask_payload(xhello2, 5);
    fmt.putn(xhello2, 5);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x8a\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58", 11));

    // 256 bytes binary message in a single unmasked frame
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = websocket_BINARY;
    header.payload_len = 256;
    header.encode(fmt);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x82\x7E\x01\x00", 4));

    // 64KiB binary message in a single unmasked frame
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = websocket_BINARY;
    header.payload_len = 65536;
    header.encode(fmt);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x82\x7F\x00\x00\x00\x00\x00\x01\x00\x00", 10));
  }
