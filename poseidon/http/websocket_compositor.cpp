// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "websocket_compositor.hpp"
#include "http_request_headers.hpp"
#include "http_response_headers.hpp"
#include "http_header_parser.hpp"
#include "../utils.hpp"
#define OPENSSL_API_COMPAT  0x10100000L
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
namespace poseidon {
namespace {

void
do_make_websocket_key(uchar_array<16>& ws_key, const void* self)
  {
    ::MD5_CTX ctx[1];
    ::MD5_Init(ctx);
    int64_t source_data[7] = { -7, -6, -5, -4, -3, ::getpid(), (intptr_t) self };
    ::MD5_Update(ctx, source_data, 56);
    ::MD5_Final(ws_key.data(), ctx);
  }

void
do_make_websocket_accept(uchar_array<20>& ws_accept, const uchar_array<25>& ws_key_str)
  {
    ::SHA_CTX ctx[1];
    ::SHA1_Init(ctx);
    ::SHA1_Update(ctx, ws_key_str.data(), 24);
    ::SHA1_Update(ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
    ::SHA1_Final(ws_accept.data(), ctx);
  }

}  // namespace

WebSocket_Compositor::
~WebSocket_Compositor()
  {
  }

void
WebSocket_Compositor::
create_handshake_request(HTTP_Request_Headers& req) const
  {
    req.method = sref("GET");
    req.uri = sref("/");
    req.headers.clear();
    req.headers.reserve(8);
    req.headers.emplace_back(sref("Connection"), sref("Upgrade"));
    req.headers.emplace_back(sref("Upgrade"), sref("websocket"));
    req.headers.emplace_back(sref("Sec-WebSocket-Version"), 13);

    uchar_array<16> ws_key;
    do_make_websocket_key(ws_key, this);
    uchar_array<25> ws_key_str;
    ::EVP_EncodeBlock(ws_key_str.data(), ws_key.data(), 16);
    req.headers.emplace_back(sref("Sec-WebSocket-Key"), cow_string((const char*) ws_key_str.data(), 24));
  }

void
WebSocket_Compositor::
accept_handshake_request(HTTP_Response_Headers& resp, const HTTP_Request_Headers& req)
  {
    resp.status = 400;
    resp.reason.clear();
    resp.headers.clear();
    resp.headers.reserve(8);
    resp.headers.emplace_back(sref("Connection"), sref("Close"));

    // Parse request headers from the client.
    bool connection_ok = false;
    bool upgrade_ok = false;
    bool ws_version_ok = false;
    uchar_array<25> ws_key_str = { };
    HTTP_Header_Parser hparser;

    for(const auto& hpair : req.headers)
      if(ascii_ci_equal(hpair.first, sref("Connection"))) {
        // Connection: Upgrade
        if(!hpair.second.is_string())
          continue;

        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("Close")))
            return;
          else if(ascii_ci_equal(hparser.current_name(), sref("Upgrade")))
            connection_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Upgrade"))) {
        // Upgrade: websocket
        if(!hpair.second.is_string())
          continue;

        if(ascii_ci_equal(hpair.second.as_string(), sref("websocket")))
          upgrade_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Version"))) {
        // Sec-WebSocket-Version: 13
        if(!hpair.second.is_number())
          continue;

        if(hpair.second.as_number() != 13) {
          // Respond with `426 Upgrade Required` if `Sec-WebSocket-Version` does
          // not match the version we use.
          // Reference: https://datatracker.ietf.org/doc/html/rfc6455#section-4.2.2
          resp.status = 426;
          resp.headers.emplace_back(sref("Sec-WebSocket-Version"), 13);
          return;
        }
        else
          ws_version_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Key"))) {
        // Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
        if(!hpair.second.is_string())
          continue;

        if(hpair.second.as_string().length() == 24)
          ::memcpy(ws_key_str.data(), hpair.second.as_string().c_str(), 25);
      }

    if(!connection_ok || !upgrade_ok || !ws_version_ok || !ws_key_str[0])
      return;

    // Compose the response.
    resp.status = 101;
    resp.headers.clear();
    resp.headers.emplace_back(sref("Connection"), sref("Upgrade"));
    resp.headers.emplace_back(sref("Upgrade"), sref("websocket"));

    uchar_array<20> ws_accept;
    do_make_websocket_accept(ws_accept, ws_key_str);
    uchar_array<29> ws_accept_str;
    ::EVP_EncodeBlock(ws_accept_str.data(), ws_accept.data(), 20);
    resp.headers.emplace_back(sref("Sec-WebSocket-Accept"), cow_string((const char *) ws_accept_str.data(), 28));
  }

void
WebSocket_Compositor::
accept_handshake_response(const HTTP_Response_Headers& resp)
  {
    this->m_data_opcode = error_opcode;

    // The response must be `101 Switching Protocol`.
    if(resp.status != 101)
      return;

    // Parse request headers from the server.
    bool connection_ok = false;
    bool upgrade_ok = false;
    uchar_array<29> ws_accept_str = { };
    HTTP_Header_Parser hparser;

    for(const auto& hpair : resp.headers)
      if(ascii_ci_equal(hpair.first, sref("Connection"))) {
        // Connection: Upgrade
        if(!hpair.second.is_string())
          continue;

        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("Close")))
            return;
          else if(ascii_ci_equal(hparser.current_name(), sref("Upgrade")))
            connection_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Upgrade"))) {
        // Upgrade: websocket
        if(!hpair.second.is_string())
          continue;

        if(ascii_ci_equal(hpair.second.as_string(), sref("websocket")))
          upgrade_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Accept"))) {
        // Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
        if(!hpair.second.is_string())
          continue;

        if(hpair.second.as_string().length() == 28)
          ::memcpy(ws_accept_str.data(), hpair.second.as_string().c_str(), 29);
      }

    if(!connection_ok || !upgrade_ok || !ws_accept_str[0])
      return;

    // Validate the key response.
    uchar_array<16> ws_key;
    do_make_websocket_key(ws_key, this);
    uchar_array<25> ws_key_str;
    ::EVP_EncodeBlock(ws_key_str.data(), ws_key.data(), 16);
    uchar_array<20> ws_accept;
    do_make_websocket_accept(ws_accept, ws_key_str);

    // The size of output of `EVP_DecodeBlock()` is always a multiple of three
    // due to padding. The extra zero byte will be ignored.
    uchar_array<21> ws_accept_from_server;
    if(::EVP_DecodeBlock(ws_accept_from_server.data(), ws_accept_str.data(), 28) != 21)
      return;
    else if(::memcmp(ws_accept.data(), ws_accept_from_server.data(), 20) != 0)
      return;

    // Clear the error status to enable the parser.
    this->m_data_opcode = 0;
  }

}  // namespace
