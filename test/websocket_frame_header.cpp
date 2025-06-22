// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

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
    header.opcode = ws_TEXT;
    header.payload_len = 5;
    header.encode(fmt);
    fmt.putn("Hello", 5);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x81\x05\x48\x65\x6c\x6c\x6f", 7));

    // A single-frame masked text message
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = ws_TEXT;
    header.masked = 1;
    header.masking_key = 0x37fa213d;
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

    header.opcode = ws_TEXT;
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
    header.opcode = ws_PING;
    header.payload_len = 5;
    header.encode(fmt);
    fmt.putn("Hello", 5);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x89\x05\x48\x65\x6c\x6c\x6f", 7));

    // 2
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = ws_PONG;
    header.masked = 1;
    header.masking_key = 0x37fa213d;
    header.payload_len = 5;
    header.encode(fmt);

    char xhello2[] = "Hello";
    header.mask_payload(xhello2, 5);
    fmt.putn(xhello2, 5);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x8a\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58", 11));

    // 3
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = ws_PONG;
    header.masked = 1;
    header.masking_key = 0x37fa213d;
    header.payload_len = 283;
    header.encode(fmt);

    char xhello3[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
                     "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
                     "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
                     "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate";
    header.mask_payload(xhello3, 283);
    fmt.putn(xhello3, 283);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(),
        "\x8a\xfe\x01\x1b\x37\xfa\x21\x3d\x7b\x95\x53\x58\x5a\xda\x48\x4d\x44\x8f\x4c\x1d\x53"
        "\x95\x4d\x52\x45\xda\x52\x54\x43\xda\x40\x50\x52\x8e\x0d\x1d\x54\x95\x4f\x4e\x52\x99"
        "\x55\x58\x43\x8f\x53\x1d\x56\x9e\x48\x4d\x5e\x89\x42\x54\x59\x9d\x01\x58\x5b\x93\x55"
        "\x11\x17\x89\x44\x59\x17\x9e\x4e\x1d\x52\x93\x54\x4e\x5a\x95\x45\x1d\x43\x9f\x4c\x4d"
        "\x58\x88\x01\x54\x59\x99\x48\x59\x5e\x9e\x54\x53\x43\xda\x54\x49\x17\x96\x40\x5f\x58"
        "\x88\x44\x1d\x52\x8e\x01\x59\x58\x96\x4e\x4f\x52\xda\x4c\x5c\x50\x94\x40\x1d\x56\x96"
        "\x48\x4c\x42\x9b\x0f\x1d\x62\x8e\x01\x58\x59\x93\x4c\x1d\x56\x9e\x01\x50\x5e\x94\x48"
        "\x50\x17\x8c\x44\x53\x5e\x9b\x4c\x11\x17\x8b\x54\x54\x44\xda\x4f\x52\x44\x8e\x53\x48"
        "\x53\xda\x44\x45\x52\x88\x42\x54\x43\x9b\x55\x54\x58\x94\x01\x48\x5b\x96\x40\x50\x54"
        "\x95\x01\x51\x56\x98\x4e\x4f\x5e\x89\x01\x53\x5e\x89\x48\x1d\x42\x8e\x01\x5c\x5b\x93"
        "\x50\x48\x5e\x8a\x01\x58\x4f\xda\x44\x5c\x17\x99\x4e\x50\x5a\x95\x45\x52\x17\x99\x4e"
        "\x53\x44\x9f\x50\x48\x56\x8e\x0f\x1d\x73\x8f\x48\x4e\x17\x9b\x54\x49\x52\xda\x48\x4f"
        "\x42\x88\x44\x1d\x53\x95\x4d\x52\x45\xda\x48\x53\x17\x88\x44\x4d\x45\x9f\x49\x58\x59"
        "\x9e\x44\x4f\x5e\x8e\x01\x54\x59\xda\x57\x52\x5b\x8f\x51\x49\x56\x8e\x44", 291));

    // 4
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = ws_PONG;
    header.masked = 1;
    header.masking_key = 0x37fa213d;
    header.payload_len = 5;
    header.encode(fmt);

    for(uint32_t i = 0; i < 21; ++i) {
      ::strcpy(xhello3, "abcdefghijklmn");
      header.mask_payload(xhello3, i);
      fmt.putn(xhello3, i);
    }

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(),
        "\x8a\x85\x37\xfa\x21\x3d\x56\x9b\x43\x5c\x55\x99\x40\x5f\x54\x9e\x40\x5f\x54\x9e\x44"
        "\x5c\x55\x99\x45\x58\x51\x9b\x43\x5e\x53\x9f\x47\x5a\x56\x98\x42\x59\x52\x9c\x46\x55"
        "\x56\x98\x42\x59\x52\x9c\x46\x55\x5e\x9b\x43\x5e\x53\x9f\x47\x5a\x5f\x93\x4b\x5c\x55"
        "\x99\x45\x58\x51\x9d\x49\x54\x5d\x91\x40\x5f\x54\x9e\x44\x5b\x50\x92\x48\x57\x5c\x96"
        "\x40\x5f\x54\x9e\x44\x5b\x50\x92\x48\x57\x5c\x96\x4c\x5c\x55\x99\x45\x58\x51\x9d\x49"
        "\x54\x5d\x91\x4d\x50\x59\x9b\x43\x5e\x53\x9f\x47\x5a\x5f\x93\x4b\x56\x5b\x97\x4f\x3d"
        "\x56\x98\x42\x59\x52\x9c\x46\x55\x5e\x90\x4a\x51\x5a\x94\x21\x6f\x56\x98\x42\x59\x52"
        "\x9c\x46\x55\x5e\x90\x4a\x51\x5a\x94\x21\x52\x72\x9b\x43\x5e\x53\x9f\x47\x5a\x5f\x93"
        "\x4b\x56\x5b\x97\x4f\x3d\x65\x88\xfb\x5c\x55\x99\x45\x58\x51\x9d\x49\x54\x5d\x91\x4d"
        "\x50\x59\xfa\x44\xb5\xcc\xa8\x40\x5f\x54\x9e\x44\x5b\x50\x92\x48\x57\x5c\x96\x4c\x53"
        "\x37\xbe\x94\xf1\x9f\xae", 217));

    // 256 bytes binary message in a single unmasked frame
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = ws_BINARY;
    header.payload_len = 256;
    header.encode(fmt);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x82\x7E\x01\x00", 4));

    // 64KiB binary message in a single unmasked frame
    fmt.clear_string();
    header.clear();

    header.fin = 1;
    header.opcode = ws_BINARY;
    header.payload_len = 65536;
    header.encode(fmt);

    POSEIDON_TEST_CHECK(xmemeq(fmt.c_str(), "\x82\x7F\x00\x00\x00\x00\x00\x01\x00\x00", 10));
  }
