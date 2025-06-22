// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "websocket_frame_parser.hpp"
#include "http_c_headers.hpp"
#include "http_s_headers.hpp"
#include "http_header_parser.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../socket/enums.hpp"
#include "../utils.hpp"
#define OPENSSL_API_COMPAT  0x10100000L
#include <openssl/sha.h>
#include <openssl/evp.h>
namespace poseidon {
namespace {

struct Sec_WebSocket
  {
    char key_str[25] = "";  // ceil(16 / 3) * 4 + 1
    char key_padding[3];

    char accept_str[29] = "";  // ceil(20 / 3) * 4 + 1
    char accept_padding[3];

    void
    make_key_str(const void* self)
      {
        ::SHA_CTX ctx;
        ::SHA1_Init(&ctx);
        int64_t source_data[7] = { -7, ::getpid(), -6, reinterpret_cast<intptr_t>(self) };
        ::SHA1_Update(&ctx, source_data, 56);
        unsigned char checksum[20];
        ::SHA1_Final(checksum, &ctx);
        ::EVP_EncodeBlock(reinterpret_cast<unsigned char*>(this->key_str), checksum, 16);
      }

    void
    make_accept_str()
      {
        ::SHA_CTX ctx;
        ::SHA1_Init(&ctx);
        ::SHA1_Update(&ctx, this->key_str, 24);
        ::SHA1_Update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
        unsigned char checksum[20];
        ::SHA1_Final(checksum, &ctx);
        ::EVP_EncodeBlock(reinterpret_cast<unsigned char*>(this->accept_str), checksum, 20);
      }
  };

struct PerMessage_Deflate
  {
    uint8_t compression_level = 0;
    bool server_no_context_takeover = false;
    bool client_no_context_takeover = false;
    char reserved;
    int server_max_window_bits = 15;
    int client_max_window_bits = 15;

    void
    use_permessage_deflate(HTTP_Header_Parser& hparser, int default_compression_level)
      {
        if(this->compression_level != 0)
          return;

        if(default_compression_level == 0)
          return;

        // Set default parameters, so in case of errors, we return immediately.
        this->server_no_context_takeover = false;
        this->client_no_context_takeover = false;
        this->server_max_window_bits = 15;
        this->client_max_window_bits = 15;

        // PMCE is accepted only if all parameters are valid and accepted.
        // Reference: https://datatracker.ietf.org/doc/html/rfc7692
        while(hparser.next_attribute())
          if(hparser.current_name() == "server_no_context_takeover") {
            if(!hparser.current_value().is_null())
              return;

            // `server_no_context_takeover`:
            // States that the server will not reuse a previous LZ77 sliding
            // window when compressing a message. Ignored by clients.
            this->server_no_context_takeover = true;
          }
          else if(hparser.current_name() == "client_no_context_takeover") {
            if(!hparser.current_value().is_null())
              return;

            // `client_no_context_takeover`:
            // States that the client will not reuse a previous LZ77 sliding
            // window when compressing a message. Ignored by servers.
            this->client_no_context_takeover = true;
          }
          else if(hparser.current_name() == "server_max_window_bits") {
            if(hparser.current_value().is_null())
              continue;

            // `server_max_window_bits`:
            // States the maximum size of the LZ77 sliding window that the server
            // will use, in number of bits.
            int64_t value = hparser.current_value().as_integer();
            if((value < 9) || (value > 15))
              return;

            this->server_max_window_bits = static_cast<int>(value);
          }
          else if(hparser.current_name() == "client_max_window_bits") {
            if(hparser.current_value().is_null())
              continue;

            // `client_max_window_bits`:
            // States the maximum size of the LZ77 sliding window that the client
            // will use, in number of bits.
            int64_t value = hparser.current_value().as_integer();
            if((value < 9) || (value > 15))
              return;

            this->client_max_window_bits = static_cast<int>(value);
          }
          else
            return;

        // If all parameters have been acepted, accept this PMCE specification.
        this->compression_level = static_cast<uint8_t>(default_compression_level);
      }
  };

}  // namespace

WebSocket_Frame_Parser::
WebSocket_Frame_Parser()
  {
    const auto conf_file = main_config.copy();
    auto conf_value = conf_file.query(&"network.http.default_compression_level");
    if(conf_value.is_integer())
      this->m_default_compression_level = clamp_cast<int>(conf_value.as_integer(), 0, 9);
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.http.default_compression_level`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    conf_value = conf_file.query(&"network.http.max_websocket_message_length");
    if(conf_value.is_integer())
      this->m_max_message_length = clamp_cast<uint32_t>(conf_value.as_integer(), 0x100, 0x10000000);
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.http.max_websocket_message_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());
  }

WebSocket_Frame_Parser::
~WebSocket_Frame_Parser()
  {
  }

void
WebSocket_Frame_Parser::
clear() noexcept
  {
    this->m_frm_header.clear();
    this->m_frm_payload.clear();
    this->m_frm_payload_rem = 0;

    this->m_state_stor = 0;
    this->m_error_desc = nullptr;
  }

void
WebSocket_Frame_Parser::
deallocate() noexcept
  {
    ::rocket::exchange(this->m_frm_payload);
  }

void
WebSocket_Frame_Parser::
create_handshake_request(HTTP_C_Headers& req)
  {
    if((this->m_wshs != wshs_pending) && (this->m_wshs != wshs_c_req_sent))
      POSEIDON_THROW((
          "`create_handshake_request()` must be called at very first (state `$1` not valid)"),
          this->m_wshs);

    // Compose the handshake request.
    req.clear();
    req.method = http_GET;
    req.raw_path = &"/";
    req.headers.reserve(8);
    req.headers.emplace_back(&"Connection", &"Upgrade");
    req.headers.emplace_back(&"Upgrade", &"websocket");
    req.headers.emplace_back(&"Sec-WebSocket-Version", 13);

    Sec_WebSocket sec_ws;
    sec_ws.make_key_str(this);
    req.headers.emplace_back(&"Sec-WebSocket-Key", cow_string(sec_ws.key_str, 24));
    req.headers.emplace_back(&"Sec-WebSocket-Extensions",
                             &"permessage-deflate; client_max_window_bits");

    // Await the response. This cannot fail, so `m_wsf` is not updated.
    this->m_wshs = wshs_c_req_sent;
  }

void
WebSocket_Frame_Parser::
accept_handshake_request(HTTP_S_Headers& resp, const HTTP_C_Headers& req)
  {
    if(this->m_wshs != wshs_pending)
      POSEIDON_THROW((
          "`accept_handshake_request()` must be called at very first (state `$1` not valid)"),
          this->m_wshs);

    // Compose a default response; So in case of errors, we return immediately.
    resp.clear();
    resp.status = http_status_bad_request;
    resp.headers.reserve(8);
    resp.headers.emplace_back(&"Connection", &"close");

    if(req.method == http_OPTIONS) {
      // Response with allowed methods and all CORS headers in RFC 6455.
      resp.status = http_status_no_content;
      resp.headers.reserve(8);
      resp.headers.emplace_back(&"Allow", &"GET");
      resp.headers.emplace_back(&"Date", system_clock::now());
      resp.headers.emplace_back(&"Access-Control-Allow-Origin", &"*");
      resp.headers.emplace_back(&"Access-Control-Allow-Methods", &"GET");
      resp.headers.emplace_back(&"Access-Control-Allow-Headers",
                                &"Upgrade, Origin, Sec-WebSocket-Version, Sec-WebSocket-Key, "
                                 "Sec-WebSocket-Extensions, Sec-WebSocket-Protocol");
      return;
    }

    this->m_wsf = wsf_error;
    this->m_error_desc = "handshake request invalid";

    // Parse request headers from the client.
    HTTP_Header_Parser hparser;

    bool upgrade_ok = false;
    bool ws_version_ok = false;
    Sec_WebSocket sec_ws;
    PerMessage_Deflate pmce;

    for(const auto& hr : req.headers)
      if(hr.first == "Connection") {
        // Connection: close
        hparser.reload(hr.second.as_string());
        while(hparser.next_element())
          if(hparser.current_name() == "close")
            return;
      }
      else if(hr.first == "Upgrade") {
        // Upgrade: websocket
        if(hr.second.as_string() == "websocket")
          upgrade_ok = true;
      }
      else if(hr.first == "Sec-WebSocket-Version") {
        // Sec-WebSocket-Version: 13
        if(hr.second.as_string() == "13")
          ws_version_ok = true;
      }
      else if(hr.first == "Sec-WebSocket-Key") {
        // Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
        if(hr.second.as_string_length() == 24)
          ::memcpy(sec_ws.key_str, hr.second.as_string_c_str(), 25);
      }
      else if(hr.first == "Sec-WebSocket-Extensions") {
        // Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
        hparser.reload(hr.second.as_string());
        while(hparser.next_element())
          if(hparser.current_name() == "permessage-deflate")
            pmce.use_permessage_deflate(hparser, this->m_default_compression_level);
      }

    if(!upgrade_ok || !ws_version_ok || !sec_ws.key_str[0]) {
      // Respond with `426 Upgrade Required`.
      // Reference: https://datatracker.ietf.org/doc/html/rfc6455#section-4.2.2
      resp.status = http_status_upgrade_required;
      resp.headers.emplace_back(&"Upgrade", &"websocket");
      resp.headers.emplace_back(&"Sec-WebSocket-Version", 13);
      POSEIDON_LOG_ERROR(("WebSocket handshake request not valid; failing"));
      return;
    }

    // Compose the response.
    resp.status = http_status_switching_protocols;
    resp.headers.clear();
    resp.headers.emplace_back(&"Connection", &"Upgrade");
    resp.headers.emplace_back(&"Upgrade", &"websocket");
    resp.headers.emplace_back(&"Date", system_clock::now());
    resp.headers.emplace_back(&"Expires", &"0");

    sec_ws.make_accept_str();
    resp.headers.emplace_back(&"Sec-WebSocket-Accept", cow_string(sec_ws.accept_str, 28));

    if(pmce.compression_level != 0) {
      // If `client_no_context_takeover` is specified, it is echoed back to the
      // client. If negotiation has selected a different window size, notify it.
      // If the client did not send `server_max_window_bits`, then a default size
      // of 15 is used, which is not sent back, as the client will not accept it.
      tinyfmt_str pmce_fmt;
      pmce_fmt << "permessage-deflate";

      if(pmce.client_no_context_takeover)
        pmce_fmt << "; client_no_context_takeover";

      if(pmce.server_max_window_bits != 15)
        pmce_fmt << "; server_max_window_bits=" << pmce.server_max_window_bits;

      if(pmce.client_max_window_bits != 15)
        pmce_fmt << "; client_max_window_bits=" << pmce.client_max_window_bits;

      // Append PMCE response.
      resp.headers.emplace_back(&"Sec-WebSocket-Extensions", pmce_fmt.extract_string());

      // Accept PMCE parameters.
      this->m_pmce_reserved = 0;
      this->m_pmce_compression_level_m2 = clamp(pmce.compression_level - 2, 1, 7) & 7;
      this->m_pmce_send_no_context_takeover = pmce.server_no_context_takeover;
      this->m_pmce_send_window_bits = pmce.server_max_window_bits & 15;
      this->m_pmce_receive_window_bits = pmce.client_max_window_bits & 15;
    }

    // For the server, this connection has now been established.
    this->m_wshs = wshs_s_accepted;
    this->m_wsf = wsf_new;
    this->m_error_desc = nullptr;
  }

void
WebSocket_Frame_Parser::
accept_handshake_response(const HTTP_S_Headers& resp)
  {
    if(this->m_wshs != wshs_c_req_sent)
      POSEIDON_THROW((
          "`accept_handshake_response()` must be called after "
              "`create_handshake_request()` (state `$1` not valid)"),
          this->m_wshs);

    // Set a default state, so in case of errors, we return immediately.
    this->m_wsf = wsf_error;
    this->m_error_desc = "handshake response invalid";

    // Parse request headers from the server.
    HTTP_Header_Parser hparser;

    bool upgrade_ok = false;
    char sec_ws_accept_resp[29] = "";
    PerMessage_Deflate pmce;

    for(const auto& hr : resp.headers)
      if(hr.first == "Connection") {
        // Connection: Upgrade
        hparser.reload(hr.second.as_string());
        while(hparser.next_element())
          if(hparser.current_name() == "close")
            return;
      }
      else if(hr.first == "Upgrade") {
        // Upgrade: websocket
        if(hr.second.as_string() == "websocket")
          upgrade_ok = true;
      }
      else if(hr.first == "Sec-WebSocket-Accept") {
        // Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
        if(hr.second.as_string_length() == 28)
          ::memcpy(sec_ws_accept_resp, hr.second.as_string_c_str(), 29);
      }
      else if(hr.first == "Sec-WebSocket-Extensions") {
        // Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
        hparser.reload(hr.second.as_string());
        while(hparser.next_element())
          if(hparser.current_name() == "permessage-deflate")
            pmce.use_permessage_deflate(hparser, this->m_default_compression_level);
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

    if(pmce.compression_level != 0) {
      // Accept PMCE parameters.
      this->m_pmce_reserved = 0;
      this->m_pmce_compression_level_m2 = clamp(pmce.compression_level - 2, 1, 7) & 7;
      this->m_pmce_send_no_context_takeover = pmce.client_no_context_takeover;
      this->m_pmce_send_window_bits = pmce.client_max_window_bits & 15;
      this->m_pmce_receive_window_bits = pmce.server_max_window_bits & 15;
    }

    // For the client, this connection has now been established.
    this->m_wshs = wshs_c_accepted;
    this->m_wsf = wsf_new;
    this->m_error_desc = nullptr;
  }

void
WebSocket_Frame_Parser::
parse_frame_header_from_stream(linear_buffer& data)
  {
    if((this->m_wshs != wshs_s_accepted) && (this->m_wshs != wshs_c_accepted))
      POSEIDON_THROW((
          "WebSocket connection not established or closed (state `$1` not valid)"),
          this->m_wshs);

    if(this->m_wsf >= wsf_header_done)
      return;

    if(this->m_fin) {
      // If a previous message has finished, forget it before a next frame. It
      // is important for control frames to not touch `m_*` fields.
      this->m_fin = 0;
      this->m_rsv1 = 0;
      this->m_rsv2 = 0;
      this->m_rsv3 = 0;
      this->m_opcode = 0;
    }

    // Calculate the length of this header.
    const uint8_t* bptr = (const uint8_t*) data.data();
    size_t ntotal = 2;
    if(data.size() < ntotal)
      return;

    // Parse the first two bytes, which contain information about other fields.
    int mask_len_rsv_opcode = bptr[1] << 8 | bptr[0];

    // Unpack fields. This has to be done in an awkward way to prevent compilers
    // from doing nonsense. Sorry.
    this->m_frm_header.fin = mask_len_rsv_opcode >> 7 & 1;
    this->m_frm_header.rsv1 = mask_len_rsv_opcode >> 6 & 1;
    this->m_frm_header.rsv2 = mask_len_rsv_opcode >> 5 & 1;
    this->m_frm_header.rsv3 = mask_len_rsv_opcode >> 4 & 1;
    this->m_frm_header.opcode = static_cast<WS_Opcode>(mask_len_rsv_opcode & 15);
    this->m_frm_header.masked = mask_len_rsv_opcode >> 15 & 1;
    this->m_frm_header.reserved_1 = mask_len_rsv_opcode >> 8 & 127;

    if((this->m_wshs == wshs_s_accepted) && (this->m_frm_header.masked == 0)) {
      // RFC 6455 states that clients must masked all frames. It also requires that
      // servers must not masked frames, but we'd be permissive about unnecessary
      // masking.
      this->m_wsf = wsf_error;
      this->m_error_desc = "clients must mask frames to servers";
      return;
    }

    switch(this->m_frm_header.opcode)
      {
      case ws_TEXT:
      case ws_BINARY:
        {
          if(this->m_opcode != 0) {
            // The previous message must have terminated.
            this->m_wsf = wsf_error;
            this->m_error_desc = "continuation frame expected";
            return;
          }

          int rsv_reject = mask_len_rsv_opcode & 0b01110000;
          if(rsv_reject != 0) {
            // If PMCE has been enabled, the RSV1 bit is accepted, so remove it
            // from this mask.
            if(this->m_pmce_send_window_bits != 0)
              rsv_reject &= 0b00110000;

            if(rsv_reject != 0) {
              // Reject unknown RSV bits.
              this->m_wsf = wsf_error;
              this->m_error_desc = "invalid RSV bits in data frame";
              return;
            }
          }

          // Copy fields for later use.
          this->m_fin = this->m_frm_header.fin;
          this->m_rsv1 = this->m_frm_header.rsv1;
          this->m_rsv2 = this->m_frm_header.rsv2;
          this->m_rsv3 = this->m_frm_header.rsv3;
          this->m_opcode = this->m_frm_header.opcode;
        }

        // There shall be a message payload.
        POSEIDON_LOG_TRACE(("Data frame: opcode = $1"), this->m_opcode);
        break;

      case ws_CONTINUATION:
        {
          if(mask_len_rsv_opcode & 0b01110000) {
            // RSV bits shall only be set in the first data frame.
            this->m_wsf = wsf_error;
            this->m_error_desc = "invalid RSV bits in continuation frame";
            return;
          }

          if(this->m_opcode == 0) {
            // A continuation frame must follow a data frame.
            this->m_wsf = wsf_error;
            this->m_error_desc = "dangling continuation frame";
            return;
          }

          // If this is a FIN frame, terminate the current message.
          // If this is a FIN frame, terminate the current message.
          if(mask_len_rsv_opcode & 0b10000000)
            this->m_fin = 1;

          // There shall be a message payload.
          POSEIDON_LOG_TRACE(("Data continuation: opcode = $1"), this->m_opcode);
          break;
        }

      case ws_CLOSE:
      case ws_PING:
      case ws_PONG:
        {
          if(mask_len_rsv_opcode & 0b01110000) {
            // RSV bits shall only be set in a data frame.
            this->m_wsf = wsf_error;
            this->m_error_desc = "invalid RSV bits in control frame";
            return;
          }

          if(this->m_frm_header.payload_len > 125) {
            // RFC 6455
            // 5.5. Control Frames
            // All control frames MUST have a payload length of 125 bytes or less
            // and ...
            this->m_wsf = wsf_error;
            this->m_error_desc = "control frame length not valid";
            return;
          }

          if(this->m_frm_header.fin == 0) {
            // RFC 6455
            // 5.5. Control Frames
            // ... MUST NOT be fragmented.
            this->m_wsf = wsf_error;
            this->m_error_desc = "control frame not fragmentable";
            return;
          }

          // There shall be a message payload.
          POSEIDON_LOG_TRACE(("Control frame: opcode = $1"), this->m_opcode);
          break;
        }

      default:
        // Reject this frame with an unknown opcode.
        this->m_wsf = wsf_error;
        this->m_error_desc = "unknown opcode";
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
      this->m_frm_header.payload_len = ROCKET_BETOH16(belen);
    }
    else {
      // eight-byte length
      ntotal += 8;
      if(data.size() < ntotal)
        return;

      uint64_t belen;
      ::memcpy(&belen, bptr + ntotal - 8, 8);
      this->m_frm_header.payload_len = ROCKET_BETOH64(belen);
    }

    if(this->m_frm_header.masked) {
      // four-byte masking key
      ntotal += 4;
      if(data.size() < ntotal)
        return;

      uint32_t bekey;
      ::memcpy(&bekey, bptr + ntotal - 4, 4);
      this->m_frm_header.masking_key = ROCKET_BETOH32(bekey);
    }

    data.discard(ntotal);
    this->m_frm_payload_rem = this->m_frm_header.payload_len;

    this->m_wsf = wsf_header_done;
  }

void
WebSocket_Frame_Parser::
parse_frame_payload_from_stream(linear_buffer& data)
  {
    if((this->m_wshs != wshs_s_accepted) && (this->m_wshs != wshs_c_accepted))
      POSEIDON_THROW((
          "WebSocket connection not established or closed (state `$1` not valid)"),
          this->m_wshs);

    if(this->m_wsf >= wsf_payload_done)
      return;

    if(this->m_wsf != wsf_header_done)
      POSEIDON_THROW((
          "WebSocket header not parsed yet (state `$1` not valid)"),
          this->m_wshs);

    size_t navail = (size_t) min(data.size(), this->m_frm_payload_rem);
    if(navail != 0) {
      // Move the (maybe partial) payload from `data` into `m_frm_payload`. If the
      // payload has been masked, unmask it first.
      this->m_frm_header.mask_payload(data.mut_data(), navail);
      this->m_frm_payload.putn(data.data(), navail);
      data.discard(navail);
      this->m_frm_payload_rem -= navail;
    }

    if(this->m_frm_payload_rem != 0)
      return;

    this->m_wsf = wsf_payload_done;
  }

}  // namespace
