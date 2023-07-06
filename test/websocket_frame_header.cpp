// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

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
    header.opcode = 1;
    header.payload_len = 5;
    header.encode(fmt);
    fmt.putn("Hello", 5);

    POSEIDON_TEST_CHECK(fmt.get_string() == "\x81\x05\x48\x65\x6c\x6c\x6f");

    // A single-frame masked text message
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = 1;
    header.mask = 1;
    header.mask_key_u32 = htobe32(0x37fa213d);
    header.payload_len = 5;
    header.encode(fmt);

    char xhello1[] = "Hello";
    header.mask_payload(xhello1, 5);
    fmt.putn(xhello1, 5);

    POSEIDON_TEST_CHECK(fmt.get_string() == "\x81\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58");

    // A fragmented unmasked text message
    // 1
    fmt.clear_string();
    header.clear();

    header.opcode = 1;
    header.payload_len = 3;
    header.encode(fmt);
    fmt.putn("Hel", 3);

    POSEIDON_TEST_CHECK(fmt.get_string() == "\x01\x03\x48\x65\x6c");

    // 2
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.payload_len = 2;
    header.encode(fmt);
    fmt.putn("lo", 2);

    POSEIDON_TEST_CHECK(fmt.get_string() == "\x80\x02\x6c\x6f");

    // Unmasked Ping request and masked Ping response
    // 1
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = 9;
    header.payload_len = 5;
    header.encode(fmt);
    fmt.putn("Hello", 5);

    POSEIDON_TEST_CHECK(fmt.get_string() == "\x89\x05\x48\x65\x6c\x6c\x6f");

    // 2
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = 10;
    header.mask = 1;
    header.mask_key_u32 = htobe32(0x37fa213d);
    header.payload_len = 5;
    header.encode(fmt);

    char xhello2[] = "Hello";
    header.mask_payload(xhello2, 5);
    fmt.putn(xhello2, 5);

    POSEIDON_TEST_CHECK(fmt.get_string() == "\x8a\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58");
  }
