// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_request_headers.hpp"
#include "../poseidon/http/http_response_headers.hpp"
#include "../poseidon/http/websocket_frame_parser.hpp"
using namespace ::poseidon;

int
main()
  {
    // https://datatracker.ietf.org/doc/html/rfc6455#section-1.3
    HTTP_Request_Headers req;
    req.method = "GET";
    req.uri = sref("/chat");
    req.headers.emplace_back(sref("Host"), sref("server.example.com"));
    req.headers.emplace_back(sref("Upgrade"), sref("websocket"));
    req.headers.emplace_back(sref("Connection"), sref("Upgrade"));
    req.headers.emplace_back(sref("Sec-WebSocket-Key"), sref("dGhlIHNhbXBsZSBub25jZQ=="));
    req.headers.emplace_back(sref("Origin"), sref("http://example.com"));
    req.headers.emplace_back(sref("Sec-WebSocket-Protocol"), sref("chat, superchat"));
    req.headers.emplace_back(sref("Sec-WebSocket-Version"), 13);

    WebSocket_Frame_Parser parser;
    HTTP_Response_Headers resp;
    parser.accept_handshake_request(resp, req);
    POSEIDON_TEST_CHECK(!parser.error());
    POSEIDON_TEST_CHECK(parser.is_server_mode());
    POSEIDON_TEST_CHECK(resp.status == 101);

    for(const auto& hr : resp.headers)
      if(ascii_ci_equal(hr.first, sref("Upgrade")))
        POSEIDON_TEST_CHECK(ascii_ci_equal(hr.second.as_string(), sref("websocket")));
      else if(ascii_ci_equal(hr.first, sref("Connection")))
        POSEIDON_TEST_CHECK(ascii_ci_equal(hr.second.as_string(), sref("Upgrade")));
      else if(ascii_ci_equal(hr.first, sref("Sec-WebSocket-Accept")))
        POSEIDON_TEST_CHECK(hr.second.as_string() == sref("s3pPLMBiTxaQ9kYGzzhZRbK+xOo="));

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
