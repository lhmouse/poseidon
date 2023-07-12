// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "ws_client_session.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

WS_Client_Session::
WS_Client_Session()
  : HTTP_Client_Session()  // client constructor
  {
  }

WS_Client_Session::
~WS_Client_Session()
  {
  }

void
WS_Client_Session::
do_call_on_ws_close_once(uint16_t status, chars_proxy reason)
  {
    if(this->m_closure_notified)
      return;

    this->m_closure_notified = true;
    this->do_on_ws_close(status, reason);
    this->ws_close(1000, "");
  }

void
WS_Client_Session::
do_abstract_socket_on_closed()
  {
    this->do_call_on_ws_close_once(1006, "no CLOSE frame received");
  }

void
WS_Client_Session::
do_on_tcp_connected()
  {
    // Send a handshake request.
    HTTP_Request_Headers req;
    this->m_parser.create_handshake_request(req);
    this->http_request(::std::move(req), "");
  }

void
WS_Client_Session::
do_on_http_response_payload_stream(linear_buffer& data)
  {
    // The request payload is ignored.
    data.clear();
  }

void
WS_Client_Session::
do_on_http_response_finish(HTTP_Response_Headers&& resp, linear_buffer&& /*data*/, bool close_now)
  {
    // Accept the handshake response.
    this->m_parser.accept_handshake_response(resp);

    // If the response is a failure, close the connection.
    if(this->m_parser.error() || close_now)
      this->do_call_on_ws_close_once(1002, this->m_parser.error_description());
  }

void
WS_Client_Session::
do_on_http_upgraded_stream(linear_buffer& data, bool eof)
  {
    for(;;) {
      // If something has gone wrong, ignore further incoming data.
      if(this->m_parser.error()) {
        data.clear();
        return;
      }

      if(!this->m_parser.frame_header_complete()) {
        this->m_parser.parse_frame_header_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_ws_close_once(1002, this->m_parser.error_description());
          return;
        }

        if(!this->m_parser.frame_header_complete())
          return;

        switch(this->m_parser.frame_header().opcode) {
          case 1:  // TEXT
          case 2:  // BINARY
            // If this is a data frame, it starts a new message.
            this->m_msg.clear();
            break;

          case 0:  // CONTINUATION
          case 8:  // CLOSE
          case 9:  // PING
          case 10:  // PONG
            break;

          default:
            data.clear();
            this->do_call_on_ws_close_once(1002, "invalid opcode");
            return;
        }
      }

      if(!this->m_parser.frame_payload_complete()) {
        this->m_parser.parse_frame_payload_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_ws_close_once(1002, this->m_parser.error_description());
          return;
        }

        if(!this->m_parser.frame_payload_complete())
          return;

        // Process the payload. Control frames must be processed as a whole.
        auto& payload = this->m_parser.mut_frame_payload();

        switch(this->m_parser.frame_header().opcode) {
          case 0:  // CONTINUATION
          case 1:  // TEXT
          case 2: {  // BINARY
            // If this is a data frame or continuation, its payload is part of a
            // (potentially fragmented) data message, so combine it.
            if(this->m_msg.empty())
              this->m_msg.swap(payload);
            else
              this->m_msg.putn(payload.data(), payload.size());

            if(this->m_parser.message_opcode() == 1)
              this->do_on_ws_text_stream(this->m_msg);
            else
              this->do_on_ws_binary_stream(this->m_msg);

            if(this->m_parser.message_fin()) {
              // This is the final frame of the data message, so commit it. When
              // PMCE is enabled, the payload of a deflated message does not
              // include the final `00 00 FF FF` part, which we must add back.
              if(this->m_parser.message_opcode() == 1)
                this->do_on_ws_text(::std::move(this->m_msg));
              else
                this->do_on_ws_binary(::std::move(this->m_msg));
            }
            break;
          }

          case 8: {  // CLOSE
            uint16_t status = 1005;
            chars_proxy reason = "no status code received";

            if(payload.size() >= 2) {
              // Get the status and reason string from the payload.
              int ch = payload.getc();
              ch = ch << 8 | payload.getc();
              status = (uint16_t) ch;
              reason = payload;
            }

            this->do_call_on_ws_close_once(status, reason);
            break;
          }

          case 9:  // PING
            POSEIDON_LOG_TRACE(("WebSocket PING from `$1`: $2"), this->remote_address(), payload);
            this->do_ws_send_raw_frame(10, payload);
            break;

          case 10:  // PONG
            POSEIDON_LOG_TRACE(("WebSocket PONG from `$1`: $2"), this->remote_address(), payload);
            this->do_on_ws_pong(::std::move(payload));
            break;
        }
      }

      this->m_parser.next_frame();
      POSEIDON_LOG_TRACE(("WebSocket parser done: data.size `$1`, eof `$2`"), data.size(), eof);
    }
  }

void
WS_Client_Session::
do_on_ws_text_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_text()`, but perform some
    // security checks, so we won't be affected by compromized 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_websocket_text_message_length = 1048576;

    auto value = conf_file.query("network", "http", "max_websocket_text_message_length");
    if(value.is_integer())
      max_websocket_text_message_length = value.as_integer();
    else if(!value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.http.max_websocket_text_message_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          value, conf_file.path());

    if(max_websocket_text_message_length < 0)
      POSEIDON_THROW((
          "`network.http.max_websocket_text_message_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_websocket_text_message_length, conf_file.path());

    if(data.size() > (uint64_t) max_websocket_text_message_length)
      POSEIDON_THROW((
          "WebSocket text data message too large: `$3` > `$4`",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_websocket_text_message_length);
  }

void
WS_Client_Session::
do_on_ws_binary_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_binary()`, but perform some
    // security checks, so we won't be affected by compromized 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_websocket_binary_message_length = 1048576;

    auto value = conf_file.query("network", "http", "max_websocket_binary_message_length");
    if(value.is_integer())
      max_websocket_binary_message_length = value.as_integer();
    else if(!value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.http.max_websocket_binary_message_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          value, conf_file.path());

    if(max_websocket_binary_message_length < 0)
      POSEIDON_THROW((
          "`network.http.max_websocket_binary_message_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_websocket_binary_message_length, conf_file.path());

    if(data.size() > (uint64_t) max_websocket_binary_message_length)
      POSEIDON_THROW((
          "WebSocket binary data message too large: `$3` > `$4`",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_websocket_binary_message_length);
  }

void
WS_Client_Session::
do_on_ws_pong(linear_buffer&& data)
  {
    POSEIDON_LOG_DEBUG(("WebSocket PONG from `$1`: $2"), this->remote_address(), data);
  }

void
WS_Client_Session::
do_on_ws_close(uint16_t status, chars_proxy reason)
  {
    POSEIDON_LOG_DEBUG(("WebSocket CLOSE from `$1` (status $2): $3"), this->remote_address(), status, reason);
  }

bool
WS_Client_Session::
do_ws_send_raw_frame(uint8_t opcode, chars_proxy data)
  {
    // Compose a single frame and send it. Frames to servers must be masked.
    WebSocket_Frame_Header header;
    header.fin = 1;
    header.opcode = opcode & 15;
    header.mask = 1;
    header.mask_key_u32 = (uint32_t) generate_random_seed();
    header.mask_key[0] |= '\x80';
    header.payload_len = data.n;

    tinyfmt_ln fmt;
    header.encode(fmt);
    fmt.putn(data.p, data.n);
    auto buf = fmt.extract_buffer();
    header.mask_payload(buf.mut_end() - data.n, data.n);
    return this->tcp_send(buf);
  }

bool
WS_Client_Session::
ws_send_text(chars_proxy data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // TEXT := opcode 1
    return this->do_ws_send_raw_frame(1, data);
  }

bool
WS_Client_Session::
ws_send_binary(chars_proxy data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // BINARY := opcode 2
    return this->do_ws_send_raw_frame(2, data);
  }

bool
WS_Client_Session::
ws_ping(chars_proxy data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // The length of the payload of a control frame cannot exceed 125 bytes, so
    // the data string has to be truncated if it's too long.
    chars_proxy ctl_data(data.p, min(data.n, 125));

    // PING := opcode 9
    return this->do_ws_send_raw_frame(9, ctl_data);
  }

bool
WS_Client_Session::
ws_close(uint16_t status, chars_proxy reason) noexcept
  {
    if(!this->do_has_upgraded())
      return this->tcp_close();

    // Compose a CLOSE frame. The length of the payload of a control frame cannot
    // exceed 125 bytes, so the reason string has to be truncated if it's too long.
    static_vector<char, 125> ctl_data;
    ctl_data.push_back(static_cast<char>(status >> 8));
    ctl_data.push_back(static_cast<char>(status));
    ctl_data.append(reason.p, reason.p + min(reason.n, 123));

    bool succ = false;
    try {
      // CLOSE := opcode 9
      succ = this->do_ws_send_raw_frame(8, ctl_data);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Failed to send WebSocket CLOSE notification: $1",
          "[WebSocket client session `$1` (class `$2`)]"),
          stdex);
    }
    succ |= this->tcp_close();
    return succ;
  }

}  // namespace poseidon
