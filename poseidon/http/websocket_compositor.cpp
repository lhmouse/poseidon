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

struct Sec_WebSocket
  {
    unsigned char key[16];  // MD5_DIGEST_LENGTH
    char key_str[25] = "";  // ceil(sizeof(key) / 3) * 4 + 1
    char key_padding[3];  // 44

    unsigned char accept[20];  // SHA_DIGEST_LENGTH
    char accept_str[29] = "";  // ceil(sizeof(accept) / 3) * 4 + 1
    char accept_padding[3];  // 52

    void
    make_key_str(const void* self)
      {
        ::MD5_CTX ctx;
        ::MD5_Init(&ctx);
        int64_t source_data[7] = { -7, -6, -5, -4, -3, ::getpid(), (intptr_t) self };
        ::MD5_Update(&ctx, source_data, 56);
        ::MD5_Final(this->key, &ctx);
        ::EVP_EncodeBlock((unsigned char*) this->key_str, this->key, 16);
      }

    void
    make_accept_str()
      {
        ::SHA_CTX ctx;
        ::SHA1_Init(&ctx);
        ::SHA1_Update(&ctx, this->key_str, 24);
        ::SHA1_Update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
        ::SHA1_Final(this->accept, &ctx);
        ::EVP_EncodeBlock((unsigned char*) this->accept_str, this->accept, 20);
      }
  };

struct PerMessage_Deflate
  {
    bool enabled = false;
    bool server_no_context_takeover;
    bool client_no_context_takeover;
    int server_max_window_bits;
    int client_max_window_bits;

    void
    use_permessage_deflate(HTTP_Header_Parser& hparser)
      {
        if(this->enabled)
          return;

        // Set default parameters, so in case of errors, we return immediately.
        this->server_no_context_takeover = false;
        this->client_no_context_takeover = false;
        this->server_max_window_bits = 15;
        this->client_max_window_bits = 15;

        // PMCE is accepted only if all parameters are valid and accepted.
        // Reference: https://datatracker.ietf.org/doc/html/rfc7692
        while(hparser.next_attribute())
          if(ascii_ci_equal(hparser.current_name(), sref("server_no_context_takeover"))) {
            if(!hparser.current_value().is_null())
              return;

            // `server_no_context_takeover`:
            // States that the server will not reuse a previous LZ77 sliding
            // window when compressing a message. Ignored by clients.
            this->server_no_context_takeover = true;
          }
          else if(ascii_ci_equal(hparser.current_name(), sref("client_no_context_takeover"))) {
            if(!hparser.current_value().is_null())
              return;

            // `client_no_context_takeover`:
            // States that the client will not reuse a previous LZ77 sliding
            // window when compressing a message. Ignored by servers.
            this->client_no_context_takeover = true;
          }
          else if(ascii_ci_equal(hparser.current_name(), sref("server_max_window_bits"))) {
            if(hparser.current_value().is_null())
              continue;
            else if(!hparser.current_value().is_number())
              return;

            double value = hparser.current_value().as_number();
            if(!(value >= 9) || !(value <= 15) || (value != (int) value))
              return;

            // `server_max_window_bits`:
            // States the maximum size of the LZ77 sliding window that the server
            // will use, in number of bits.
            this->server_max_window_bits = (int) value;
          }
          else if(ascii_ci_equal(hparser.current_name(), sref("client_max_window_bits"))) {
            if(hparser.current_value().is_null())
              continue;
            else if(!hparser.current_value().is_number())
              return;

            double value = hparser.current_value().as_number();
            if(!(value >= 9) || !(value <= 15) || (value != (int) value))
              return;

            // `client_max_window_bits`:
            // States the maximum size of the LZ77 sliding window that the client
            // will use, in number of bits.
            this->client_max_window_bits = (int) value;
          }
          else
            return;

        // If all parameters have been acepted, accept this PMCE specification.
        // The caller shall initialize a deflator and inflator accordingly.
        this->enabled = true;
      }
  };

}  // namespace

WebSocket_Compositor::
~WebSocket_Compositor()
  {
  }

void
WebSocket_Compositor::
create_handshake_request(HTTP_Request_Headers& req)
  {
    if(this->m_wsc != wsc_pending)
      POSEIDON_THROW((
          "`create_handshake_request()` must be called as the first function (state `$1` not valid)"),
          this->m_wsc);

    // Compose the handshake request.
    req.method = sref("GET");
    req.uri = sref("/");
    req.headers.clear();
    req.headers.reserve(8);
    req.headers.emplace_back(sref("Connection"), sref("Upgrade"));
    req.headers.emplace_back(sref("Upgrade"), sref("websocket"));
    req.headers.emplace_back(sref("Sec-WebSocket-Version"), 13);

    Sec_WebSocket sec_ws;
    sec_ws.make_key_str(this);
    req.headers.emplace_back(sref("Sec-WebSocket-Key"), cow_string(sec_ws.key_str, 24));

    req.headers.emplace_back(sref("Sec-WebSocket-Extensions"), sref("permessage-deflate; client_max_window_bits"));

    // Await the response.
    this->m_wsc = wsc_c_req_sent;
  }

void
WebSocket_Compositor::
accept_handshake_request(HTTP_Response_Headers& resp, const HTTP_Request_Headers& req)
  {
    if(this->m_wsc != wsc_pending)
      POSEIDON_THROW((
          "`accept_handshake_request()` must be called as the first function (state `$1` not valid)"),
          this->m_wsc);

    // Compose a default response, so in case of errors, we return immediately.
    resp.status = 400;
    resp.reason.clear();
    resp.headers.clear();
    resp.headers.reserve(8);
    resp.headers.emplace_back(sref("Connection"), sref("close"));

    this->m_wsc = wsc_error;

    // Parse request headers from the client.
    HTTP_Header_Parser hparser;

    bool upgrade_ok = false;
    bool ws_version_ok = false;
    Sec_WebSocket sec_ws;
    PerMessage_Deflate pmce;

    for(const auto& hpair : req.headers)
      if(ascii_ci_equal(hpair.first, sref("Connection"))) {
        if(!hpair.second.is_string())
          continue;

        // Connection: Upgrade
        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("close")))
            return;
      }
      else if(ascii_ci_equal(hpair.first, sref("Upgrade"))) {
        if(!hpair.second.is_string())
          continue;

        // Upgrade: websocket
        if(ascii_ci_equal(hpair.second.as_string(), sref("websocket")))
          upgrade_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Version"))) {
        if(!hpair.second.is_number())
          continue;

        // Sec-WebSocket-Version: 13
        if(hpair.second.as_number() == 13)
          ws_version_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Key"))) {
        if(!hpair.second.is_string())
          continue;

        // Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
        if(hpair.second.as_string().length() == 24)
          ::memcpy(sec_ws.key_str, hpair.second.as_c_str(), 25);
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Extensions"))) {
        if(!hpair.second.is_string())
          continue;

        // Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("permessage-deflate")))
            pmce.use_permessage_deflate(hparser);
          else
            return;  // unknown extension; ignore
      }

    if(!upgrade_ok || !ws_version_ok || !sec_ws.key_str[0]) {
      // Respond with `426 Upgrade Required`.
      // Reference: https://datatracker.ietf.org/doc/html/rfc6455#section-4.2.2
      resp.status = 426;
      resp.headers.emplace_back(sref("Upgrade"), sref("websocket"));
      resp.headers.emplace_back(sref("Sec-WebSocket-Version"), 13);
      POSEIDON_LOG_ERROR(("WebSocket handshake request not valid; failing"));
      return;
    }

    // Compose the response.
    resp.status = 101;
    resp.headers.clear();
    resp.headers.emplace_back(sref("Connection"), sref("Upgrade"));
    resp.headers.emplace_back(sref("Upgrade"), sref("websocket"));

    sec_ws.make_accept_str();
    resp.headers.emplace_back(sref("Sec-WebSocket-Accept"), cow_string(sec_ws.accept_str, 28));

    if(pmce.enabled) {
      // Append PMCE response parameters. `client_no_context_takeover` and
      // `client_max_window_bits` are ignored and unnecessary to be sent back to
      // the client.
      tinyfmt_str pmce_resp_fmt;
      pmce_resp_fmt.set_string(sref("permessage-deflate"));

      if(pmce.server_no_context_takeover)
        pmce_resp_fmt << "; server_no_context_takeover";

      if(pmce.server_max_window_bits != 15)
        pmce_resp_fmt << "; server_max_window_bits=" << pmce.server_max_window_bits;

      // Initialize data compressor and decompressor.
    }

    // For the server, this connection has now been established.
    this->m_wsc = wsc_s_accepted;
  }

void
WebSocket_Compositor::
accept_handshake_response(const HTTP_Response_Headers& resp)
  {
    if(this->m_wsc != wsc_c_req_sent)
      POSEIDON_THROW((
          "`accept_handshake_response()` must be called after `create_handshake_request()` (state `$1` not valid)"),
          this->m_wsc);

    // Set a default state, so in case of errors, we return immediately.
    this->m_wsc = wsc_error;

    // Parse request headers from the server.
    HTTP_Header_Parser hparser;

    bool upgrade_ok = false;
    char sec_ws_accept_resp[29] = "";
    PerMessage_Deflate pmce;

    for(const auto& hpair : resp.headers)
      if(ascii_ci_equal(hpair.first, sref("Connection"))) {
        if(!hpair.second.is_string())
          continue;

        // Connection: Upgrade
        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("close")))
            return;
      }
      else if(ascii_ci_equal(hpair.first, sref("Upgrade"))) {
        if(!hpair.second.is_string())
          continue;

        // Upgrade: websocket
        if(ascii_ci_equal(hpair.second.as_string(), sref("websocket")))
          upgrade_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Accept"))) {
        if(!hpair.second.is_string())
          continue;

        // Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
        if(hpair.second.as_string().length() == 28)
          ::memcpy(sec_ws_accept_resp, hpair.second.as_c_str(), 29);
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Extensions"))) {
        if(!hpair.second.is_string())
          continue;

        // Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("permessage-deflate")))
            pmce.use_permessage_deflate(hparser);
          else
            return;  // unknown extension; fail
      }

    if(!upgrade_ok || !sec_ws_accept_resp[0]) {
      // Don't send a closure notification.
      POSEIDON_LOG_ERROR(("WebSocket handshake response not valid; failing"));
      return;
    }

    // Rebuild the handshake response and compare it.
    Sec_WebSocket sec_ws;
    sec_ws.make_key_str(this);
    sec_ws.make_accept_str();

    // In theory, the final base64-block of the key response contains two padding
    // bits which shouldn't be examined, should they?
    if(::memcmp(sec_ws.accept_str, sec_ws_accept_resp, 28) != 0)
      return;

    if(pmce.enabled) {
      // Initialize data compressor and decompressor.
    }

    // For the server, this connection has now been established.
    this->m_wsc = wsc_c_accepted;
  }

}  // namespace
