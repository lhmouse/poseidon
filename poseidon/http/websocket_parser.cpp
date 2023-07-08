// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "websocket_parser.hpp"
#include "http_request_headers.hpp"
#include "http_response_headers.hpp"
#include "http_header_parser.hpp"
#include "../utils.hpp"
#define OPENSSL_API_COMPAT  0x10100000L
#include <openssl/sha.h>
#include <openssl/evp.h>
namespace poseidon {

WebSocket_Parser::
~WebSocket_Parser()
  {
  }

void
WebSocket_Parser::
create_handshake_request(HTTP_Request_Headers& req) const
  {
    req.method = sref("GET");
    req.uri = sref("/");
    req.headers.clear();
    req.headers.reserve(8);
    req.headers.emplace_back(sref("Connection"), sref("Upgrade"));
    req.headers.emplace_back(sref("Upgrade"), sref("websocket"));
    req.headers.emplace_back(sref("Sec-WebSocket-Version"), 13);

    // The key is a truncated checksum of the current process ID and the address
    // of the current parser object.
    ::SHA_CTX sha[1];
    uint64_t value;
    uint8_t checksum[20], ws_key[25];

    ::SHA1_Init(sha);
    value = (uint32_t) ::getpid();
    ::SHA1_Update(sha, &value, sizeof(value));
    value = (uintptr_t) this;
    ::SHA1_Update(sha, &value, sizeof(value));
    ::SHA1_Final(checksum, sha);
    ::EVP_EncodeBlock(ws_key, checksum, 16);

    req.headers.emplace_back(sref("Sec-WebSocket-Key"), cow_string((const char*) ws_key, 24));
  }

void
WebSocket_Parser::
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
    cow_string ws_key_req;
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
          ws_key_req = hpair.second.as_string();
      }

    if(!connection_ok || !upgrade_ok || !ws_version_ok || ws_key_req.empty())
      return;

    // Compose the response.
    resp.status = 101;
    resp.headers.clear();
    resp.headers.emplace_back(sref("Connection"), sref("Upgrade"));
    resp.headers.emplace_back(sref("Upgrade"), sref("websocket"));

    // Calculate the key response.
    ::SHA_CTX sha[1];
    uint8_t checksum[20], ws_accept[29];

    ::SHA1_Init(sha);
    ::SHA1_Update(sha, ws_key_req.data(), ws_key_req.size());
    ::SHA1_Update(sha, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
    ::SHA1_Final(checksum, sha);
    ::EVP_EncodeBlock(ws_accept, checksum, 20);

    resp.headers.emplace_back(sref("Sec-WebSocket-Accept"), cow_string((const char*) ws_accept, 28));
  }

void
WebSocket_Parser::
accept_handshake_response(const HTTP_Response_Headers& resp)
  {
    this->m_data_opcode = error_opcode;

    // The response must be `101 Switching Protocol`.
    if(resp.status != 101)
      return;

    // Parse request headers from the server.
    bool connection_ok = false;
    bool upgrade_ok = false;
    cow_string ws_acc_resp;
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
          ws_acc_resp = hpair.second.as_string();
      }

    if(!connection_ok || !upgrade_ok || ws_acc_resp.empty())
      return;

    // Validate the key response. The key was a truncated checksum of the current
    // process ID and the address of the current parser object.
    ::SHA_CTX sha[1];
    uint64_t value;
    uint8_t checksum[20], ws_key[25], ws_acc[29];

    ::SHA1_Init(sha);
    value = (uint32_t) ::getpid();
    ::SHA1_Update(sha, &value, sizeof(value));
    value = (uintptr_t) this;
    ::SHA1_Update(sha, &value, sizeof(value));
    ::SHA1_Final(checksum, sha);
    ::EVP_EncodeBlock(ws_key, checksum, 16);

    ::SHA1_Init(sha);
    ::SHA1_Update(sha, ws_key, 24);
    ::SHA1_Update(sha, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
    ::SHA1_Final(checksum, sha);
    ::EVP_EncodeBlock(ws_acc, checksum, 20);

    if(::memcmp(ws_acc_resp.data(), ws_acc, 28) != 0)
      return;

    // Clear the error status. WebSocket frames can be sent now.
    this->m_data_opcode = 0;
  }

}  // namespace
