// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_c_headers.hpp"
#include "../poseidon/http/http_s_headers.hpp"
#include "../poseidon/http/websocket_frame_parser.hpp"
using namespace ::poseidon;

int
main()
  {
    // https://datatracker.ietf.org/doc/html/rfc6455#section-1.3
    HTTP_C_Headers req;
    req.method = http_GET;
    req.raw_path = &"/chat";
    req.headers.emplace_back(&"Host", &"server.example.com");
    req.headers.emplace_back(&"Upgrade", &"websocket");
    req.headers.emplace_back(&"Connection", &"Upgrade");
    req.headers.emplace_back(&"Sec-WebSocket-Key", &"dGhlIHNhbXBsZSBub25jZQ==");
    req.headers.emplace_back(&"Origin", &"http://example.com");
    req.headers.emplace_back(&"Sec-WebSocket-Protocol", &"chat, superchat");
    req.headers.emplace_back(&"Sec-WebSocket-Version", 13);

    WebSocket_Frame_Parser parser;
    HTTP_S_Headers resp;
    parser.accept_handshake_request(resp, req);
    POSEIDON_TEST_CHECK(!parser.error());
    POSEIDON_TEST_CHECK(parser.is_server_mode());
    POSEIDON_TEST_CHECK(resp.status == 101);

    for(const auto& hr : resp.headers)
      if(hr.first == "Upgrade")
        POSEIDON_TEST_CHECK(hr.second.as_string() == "websocket");
      else if(hr.first == "Connection")
        POSEIDON_TEST_CHECK(hr.second.as_string() == "Upgrade");
      else if(hr.first == "Sec-WebSocket-Accept")
        POSEIDON_TEST_CHECK(hr.second.as_string() == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");

    // self
    req.clear();
    parser.clear();
    parser.create_handshake_request(req);
    POSEIDON_TEST_CHECK(!parser.error());

    resp.clear();
    WebSocket_Frame_Parser parser2;
    parser2.accept_handshake_request(resp, req);
    POSEIDON_TEST_CHECK(!parser2.error());
    POSEIDON_TEST_CHECK(parser2.is_server_mode());
    POSEIDON_TEST_CHECK(resp.status == 101);

    parser.accept_handshake_response(resp);
    POSEIDON_TEST_CHECK(!parser.error());
    POSEIDON_TEST_CHECK(parser.is_client_mode());
  }
