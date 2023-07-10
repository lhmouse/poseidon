// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "websocket_parser.hpp"
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
    int server_max_window_bits;
    int client_max_window_bits;
    bool server_no_context_takeover;
    bool client_no_context_takeover;
    bool enabled = false;

    void
    use_permessage_deflate(HTTP_Header_Parser& hparser)
      {
        if(this->enabled)
          return;

        // Set default parameters, so in case of errors, we return immediately.
        this->server_max_window_bits = 15;
        this->client_max_window_bits = 15;
        this->server_no_context_takeover = false;
        this->client_no_context_takeover = false;

        // PMCE is accepted only if all parameters are valid and accepted.
        // Reference: https://datatracker.ietf.org/doc/html/rfc7692
        while(hparser.next_attribute())
          if(ascii_ci_equal(hparser.current_name(), sref("server_max_window_bits"))) {
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
          else if(ascii_ci_equal(hparser.current_name(), sref("server_no_context_takeover"))) {
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
          else
            return;

        // If all parameters have been acepted, accept this PMCE specification.
        // The caller shall initialize a deflator and inflator accordingly.
        this->enabled = true;
      }
  };

}  // namespace

WebSocket_Parser::
~WebSocket_Parser()
  {
  }

void
WebSocket_Parser::
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
WebSocket_Parser::
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
    this->m_error_description = "handshake request invalid";

    // Parse request headers from the client.
    HTTP_Header_Parser hparser;

    bool upgrade_ok = false;
    bool ws_version_ok = false;
    Sec_WebSocket sec_ws;
    PerMessage_Deflate pmce;

    for(const auto& hpair : req.headers)
      if(ascii_ci_equal(hpair.first, sref("Connection"))) {
        if(!hpair.second.is_string())
          return;

        // Connection: Upgrade
        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("close")))
            return;
      }
      else if(ascii_ci_equal(hpair.first, sref("Upgrade"))) {
        if(!hpair.second.is_string())
          return;

        // Upgrade: websocket
        if(ascii_ci_equal(hpair.second.as_string(), sref("websocket")))
          upgrade_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Version"))) {
        if(!hpair.second.is_number())
          return;

        // Sec-WebSocket-Version: 13
        if(hpair.second.as_number() == 13)
          ws_version_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Key"))) {
        if(!hpair.second.is_string())
          return;

        // Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
        if(hpair.second.as_string().length() == 24)
          ::memcpy(sec_ws.key_str, hpair.second.as_c_str(), 25);
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Extensions"))) {
        if(!hpair.second.is_string())
          return;

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
      // Accept PMCE parameters.
      this->m_pmce_max_window_bits = (uint8_t) pmce.server_max_window_bits;
      this->m_pmce_no_context_takeover = pmce.server_no_context_takeover;

      // Append PMCE response parameters. If `client_no_context_takeover` and
      // `client_max_window_bits` are specified, they are echoed back to the
      // client. (Maybe this is not necessary, but we do it anyway.)
      tinyfmt_str pmce_resp_fmt;
      pmce_resp_fmt.set_string(sref("permessage-deflate"));

      if(pmce.server_max_window_bits != 15)
        pmce_resp_fmt << "; server_max_window_bits=" << pmce.server_max_window_bits;

      if(pmce.server_no_context_takeover)
        pmce_resp_fmt << "; server_no_context_takeover";

      if(pmce.client_max_window_bits != 15)
        pmce_resp_fmt << "; client_max_window_bits=" << pmce.client_max_window_bits;

      if(pmce.client_no_context_takeover)
        pmce_resp_fmt << "; client_no_context_takeover";

      resp.headers.emplace_back(sref("Sec-WebSocket-Extensions"), pmce_resp_fmt.extract_string());
    }

    // For the server, this connection has now been established.
    this->m_wsc = wsc_s_accepted;
    this->m_error_description = "";
  }

void
WebSocket_Parser::
accept_handshake_response(const HTTP_Response_Headers& resp)
  {
    if(this->m_wsc != wsc_c_req_sent)
      POSEIDON_THROW((
          "`accept_handshake_response()` must be called after `create_handshake_request()` (state `$1` not valid)"),
          this->m_wsc);

    // Set a default state, so in case of errors, we return immediately.
    this->m_wsc = wsc_error;
    this->m_error_description = "handshake response invalid";

    // Parse request headers from the server.
    HTTP_Header_Parser hparser;

    bool upgrade_ok = false;
    char sec_ws_accept_resp[29] = "";
    PerMessage_Deflate pmce;

    for(const auto& hpair : resp.headers)
      if(ascii_ci_equal(hpair.first, sref("Connection"))) {
        if(!hpair.second.is_string())
          return;

        // Connection: Upgrade
        hparser.reload(hpair.second.as_string());
        while(hparser.next_element())
          if(ascii_ci_equal(hparser.current_name(), sref("close")))
            return;
      }
      else if(ascii_ci_equal(hpair.first, sref("Upgrade"))) {
        if(!hpair.second.is_string())
          return;

        // Upgrade: websocket
        if(ascii_ci_equal(hpair.second.as_string(), sref("websocket")))
          upgrade_ok = true;
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Accept"))) {
        if(!hpair.second.is_string())
          return;

        // Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
        if(hpair.second.as_string().length() == 28)
          ::memcpy(sec_ws_accept_resp, hpair.second.as_c_str(), 29);
      }
      else if(ascii_ci_equal(hpair.first, sref("Sec-WebSocket-Extensions"))) {
        if(!hpair.second.is_string())
          return;

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
      // Accept PMCE parameters.
      this->m_pmce_max_window_bits = (uint8_t) pmce.client_max_window_bits;
      this->m_pmce_no_context_takeover = pmce.client_no_context_takeover;
    }

    // For the server, this connection has now been established.
    this->m_wsc = wsc_c_accepted;
    this->m_error_description = "";
  }

void
WebSocket_Parser::
parse_frame_header_from_stream(linear_buffer& data)
  {
    if((this->m_wsc != wsc_s_accepted) && (this->m_wsc != wsc_c_accepted))
      POSEIDON_THROW((
          "WebSocket connection not established or closed (state `$1` not valid)"),
          this->m_wsc);

    if(this->m_frm_header_complete)
      return;

    if(this->m_msg_fin) {
      // Control frames may be nested into fragmented data messages, so only
      // clear a previous message if the FIN bit has been seen.
      this->m_msg.clear();
      this->m_msg_fin = 0;
      this->m_msg_rsv1 = 0;
      this->m_msg_rsv2 = 0;
      this->m_msg_rsv3 = 0;
      this->m_msg_opcode = 0;
    }

    // Calculate the length of this header.
    const uint8_t* bptr = (const uint8_t*) data.data();
    size_t ntotal = 2;
    if(data.size() < ntotal)
      return;

    // Parse the first two bytes, which contain information about other fields.
    // This has to be done in an awkward way to prevent compilers from doing
    // nonsense. Sorry.
    int word = bptr[0] | bptr[1] << 8;

    this->m_frm_header.fin = (word >> 7) & 1;
    this->m_frm_header.rsv1 = (word >> 6) & 1;
    this->m_frm_header.rsv2 = (word >> 5) & 1;
    this->m_frm_header.rsv3 = (word >> 4) & 1;
    this->m_frm_header.opcode = word & 15;
    this->m_frm_header.mask = (word >> 15) & 1;
    this->m_frm_header.reserved_1 = (word >> 8) & 127;

    if((this->m_wsc == wsc_s_accepted) && (this->m_frm_header.mask == 0)) {
      // RFC 6455 says clients must mask all frames.
      this->m_wsc = wsc_error;
      this->m_error_description = "clients must mask frames to servers";
      return;
    }
    else if((this->m_wsc == wsc_c_accepted) && (this->m_frm_header.mask != 0)) {
      // RFC 6455 says servers must not mask any frames. Obey that, period.
      this->m_wsc = wsc_error;
      this->m_error_description = "servers must not mask frames to clients";
      return;
    }

    switch(this->m_frm_header.opcode) {
      case 0:
        // continuation
        if(this->m_msg_opcode == 0) {
          this->m_wsc = wsc_error;
          this->m_error_description = "dangling continuation frame";
          return;
        }
        else if(m_frm_header.rsv1 || m_frm_header.rsv2 || m_frm_header.rsv3) {
          this->m_wsc = wsc_error;
          this->m_error_description = "reserved bit set in continuation frame";
          return;
        }
        break;

      case 8:
      case 9:
      case 10:
        // control
        if(m_frm_header.fin == 0) {
          this->m_wsc = wsc_error;
          this->m_error_description = "control frame not fragmentable";
          return;
        }
        else if(this->m_frm_header.reserved_1 > 125) {
          this->m_wsc = wsc_error;
          this->m_error_description = "control frame too large";
          return;
        }
        else if(m_frm_header.rsv1 || m_frm_header.rsv2 || m_frm_header.rsv3) {
          this->m_wsc = wsc_error;
          this->m_error_description = "reserved bit set in control frame";
          return;
        }
        break;

      case 1:
      case 2:
        // data (text or binary)
        this->m_msg_fin = this->m_frm_header.fin;
        this->m_msg_rsv1 = this->m_frm_header.rsv1;
        this->m_msg_rsv2 = this->m_frm_header.rsv2;
        this->m_msg_rsv3 = this->m_frm_header.rsv3;
        this->m_msg_opcode = this->m_frm_header.opcode;
        break;

      default:
        this->m_wsc = wsc_error;
        this->m_error_description = "unknown opcode";
        return;
    }

    if(this->m_frm_header.reserved_1 <= 125) {
      // one-byte length
      this->m_frm_header.payload_len = m_frm_header.reserved_1;
    }
    else if(this->m_frm_header.reserved_1 == 126) {
      // two-byte length
      ntotal += 2;
      if(data.size() < ntotal)
        return;

      uint16_t belen;
      ::memcpy(&belen, bptr + ntotal - 2, 2);
      this->m_frm_header.payload_len = be16toh(belen);
    }
    else {
      // eight-byte length
      ntotal += 8;
      if(data.size() < ntotal)
        return;

      uint64_t belen;
      ::memcpy(&belen, bptr + ntotal - 8, 8);
      this->m_frm_header.payload_len = be64toh(belen);
    }

    if(this->m_frm_header.mask) {
      // Get the masking key as four bytes; not as an integer.
      ntotal += 4;
      if(data.size() < ntotal)
        return;

      ::memcpy(this->m_frm_header.mask_key, bptr + ntotal - 4, 4);
    }

    data.discard(ntotal);
    this->m_frm_payload_remaining = this->m_frm_header.payload_len;

    // Accept the frame header.
    this->m_frm_header_complete = true;
  }

void
WebSocket_Parser::
parse_frame_payload_from_stream(linear_buffer& data)
  {
    if((this->m_wsc != wsc_s_accepted) && (this->m_wsc != wsc_c_accepted))
      POSEIDON_THROW((
          "WebSocket connection not established or closed (state `$1` not valid)"),
          this->m_wsc);

    if(this->m_frm_payload_complete)
      return;

    // Unmask the payload and append it to `m_frm_payload`. If this is a data
    // frame, also append the payload to `m_msg`.
    size_t navail = (size_t) min(data.size(), this->m_frm_payload_remaining);
    this->m_frm_header.mask_payload(data.mut_data(), navail);
    this->m_frm_payload.putn(data.data(), navail);

    if(this->m_frm_header.opcode < 8)
      this->m_msg.putn(data.data(), navail);

    data.discard(navail);
    this->m_frm_payload_remaining -= navail;

    // Wait until all data of this frame have been received.
    if(this->m_frm_payload_remaining != 0)
      return;

    // Accept the frame payload.
    this->m_frm_payload_complete = true;
  }

}  // namespace
