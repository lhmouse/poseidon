// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "ws_server_session.hpp"
#include "../http/websocket_deflator.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

WS_Server_Session::
WS_Server_Session()
  {
  }

WS_Server_Session::
~WS_Server_Session()
  {
  }

void
WS_Server_Session::
do_call_on_ws_close_once(uint16_t status, chars_proxy reason)
  {
    if(this->m_closure_notified)
      return;

    this->m_closure_notified = true;
    this->do_on_ws_close(status, reason);
    this->ws_shut_down(1000, "");
  }

void
WS_Server_Session::
do_abstract_socket_on_closed()
  {
    this->do_call_on_ws_close_once(1006, "no CLOSE frame received");
  }

void
WS_Server_Session::
do_on_http_request_payload_stream(linear_buffer& data)
  {
    // The request payload is ignored.
    data.clear();
  }

void
WS_Server_Session::
do_on_http_request_finish(HTTP_Request_Headers&& req, linear_buffer&& /*data*/, bool close_now)
  {
    // Send the handshake response.
    HTTP_Response_Headers resp;
    this->m_parser.accept_handshake_request(resp, req);
    this->http_response(::std::move(resp), "");

    if(close_now || !this->m_parser.is_server_mode()) {
      const char* err_desc = close_now ? "connection closing" : this->m_parser.error_description();
      this->do_call_on_ws_close_once(1002, err_desc);
      return;
    }

    // Initialize extensions.
    if(this->m_parser.pmce_send_max_window_bits() != 0)
      this->m_pmce_opt = new_sh<WebSocket_Deflator>(this->m_parser);

    this->do_on_ws_accepted(::std::move(req.uri));
  }

void
WS_Server_Session::
do_on_http_request_error(uint32_t status)
  {
    // This error can be reported synchronously.
    HTTP_Response_Headers resp;
    resp.status = status;
    resp.headers.emplace_back(sref("Connection"), sref("close"));
    this->http_response(::std::move(resp), "");

    // Close the connection.
    this->do_call_on_ws_close_once(1002, "handshake failed");
  }

void
WS_Server_Session::
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
      }

      if(!this->m_parser.frame_payload_complete()) {
        this->m_parser.parse_frame_payload_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_ws_close_once(1002, this->m_parser.error_description());
          return;
        }

        // Process the payload. Control frames must be processed as a whole.
        auto& payload = this->m_parser.mut_frame_payload();

        switch(this->m_parser.frame_header().opcode) {
          case 1:  // TEXT
          case 2: {  // BINARY
            // If this is a data frame, it starts a new message.
            if(this->m_pmce_opt) {
              plain_mutex::unique_lock lock;
              auto& out_buf = this->m_pmce_opt->inflate_output_buffer(lock);
              out_buf.clear();
            }

            this->m_msg.clear();
            // fallthrough

          case 0:  // CONTINUATION
            if(this->m_parser.message_rsv1()) {
              // The RSV1 bit indicates that the payload is part of a compressed
              // message, so decompress it.
              if(!this->m_pmce_opt) {
                data.clear();
                this->do_call_on_ws_close_once(1007, "permessage-deflate not initialized properly");
                return;
              }

              plain_mutex::unique_lock lock;
              this->m_pmce_opt->inflate_message_stream(lock, payload);

              if(this->m_parser.message_fin())
                this->m_pmce_opt->inflate_message_finish(lock);

              // Replace the payload... nasty.
              auto& out_buf = this->m_pmce_opt->inflate_output_buffer(lock);
              payload.swap(out_buf);
            }

            // If this is a data frame or continuation, its payload is part of a
            // (potentially fragmented) data message, so combine it.
            if(this->m_msg.empty())
              this->m_msg.swap(payload);
            else
              this->m_msg.putn(payload.data(), payload.size());

            // Take this part of the message.
            switch(this->m_parser.message_opcode()) {
              case 1:  // TEXT
                this->do_on_ws_text_stream(this->m_msg);
                break;

              case 2:  // BINARY
                this->do_on_ws_binary_stream(this->m_msg);
                break;

              default:
                ROCKET_ASSERT(false);
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
            this->do_ws_send_raw_frame(0b10001010, payload);
            break;

          case 10:  // PONG
            POSEIDON_LOG_TRACE(("WebSocket PONG from `$1`: $2"), this->remote_address(), payload);
            this->do_on_ws_pong(::std::move(payload));
            break;
        }

        if(!this->m_parser.frame_payload_complete())
          return;

        // Has the final fragment been received?
        if(this->m_parser.message_fin())
          switch(this->m_parser.message_opcode()) {
            case 1:  // TEXT
              this->do_on_ws_text(::std::move(this->m_msg));
              break;

            case 2:  // BINARY
              this->do_on_ws_binary(::std::move(this->m_msg));
              break;

            default:
              ROCKET_ASSERT(false);
          }
      }

      this->m_parser.next_frame();
      POSEIDON_LOG_TRACE(("WebSocket parser done: data.size `$1`, eof `$2`"), data.size(), eof);
    }
  }

void
WS_Server_Session::
do_on_ws_accepted(cow_string&& uri)
  {
    POSEIDON_LOG_DEBUG(("Accepted WebSocket from `$1`: $2"), this->remote_address(), uri);
  }

void
WS_Server_Session::
do_on_ws_text_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_text()`, but perform some
    // security checks, so we won't be affected by compromized 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_websocket_text_message_length = 1048576;

    auto conf_value = conf_file.query("network", "http", "max_websocket_text_message_length");
    if(conf_value.is_integer())
      max_websocket_text_message_length = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.http.max_websocket_text_message_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if(max_websocket_text_message_length < 0)
      POSEIDON_THROW((
          "`network.http.max_websocket_text_message_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_websocket_text_message_length, conf_file.path());

    if(data.size() > (uint64_t) max_websocket_text_message_length)
      POSEIDON_THROW((
          "WebSocket text data message too large: `$3` > `$4`",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_websocket_text_message_length);
  }

void
WS_Server_Session::
do_on_ws_binary_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_binary()`, but perform some
    // security checks, so we won't be affected by compromized 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_websocket_binary_message_length = 1048576;

    auto conf_value = conf_file.query("network", "http", "max_websocket_binary_message_length");
    if(conf_value.is_integer())
      max_websocket_binary_message_length = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.http.max_websocket_binary_message_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if(max_websocket_binary_message_length < 0)
      POSEIDON_THROW((
          "`network.http.max_websocket_binary_message_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_websocket_binary_message_length, conf_file.path());

    if(data.size() > (uint64_t) max_websocket_binary_message_length)
      POSEIDON_THROW((
          "WebSocket binary data message too large: `$3` > `$4`",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_websocket_binary_message_length);
  }

void
WS_Server_Session::
do_on_ws_pong(linear_buffer&& data)
  {
    POSEIDON_LOG_DEBUG(("WebSocket PONG from `$1`: $2"), this->remote_address(), data);
  }

void
WS_Server_Session::
do_on_ws_close(uint16_t status, chars_proxy reason)
  {
    POSEIDON_LOG_DEBUG(("WebSocket CLOSE from `$1` (status $2): $3"), this->remote_address(), status, reason);
  }

bool
WS_Server_Session::
do_ws_send_raw_frame(int rsv_opcode, chars_proxy data)
  {
    // Compose a single frame and send it. Frames to clients will not be masked.
    WebSocket_Frame_Header header;
    header.fin = rsv_opcode >> 7 & 1;
    header.rsv1 = rsv_opcode >> 6 & 1;
    header.rsv2 = rsv_opcode >> 5 & 1;
    header.rsv3 = rsv_opcode >> 4 & 1;
    header.opcode = rsv_opcode & 15;
    header.payload_len = data.n;

    tinyfmt_ln fmt;
    header.encode(fmt);
    fmt.putn(data.p, data.n);
    return this->tcp_send(fmt);
  }

bool
WS_Server_Session::
do_ws_send_raw_data_frame(int rsv_opcode, chars_proxy data)
  {
    // When PMCE is not active, send the frame as is.
    if(!this->m_pmce_opt)
      return this->do_ws_send_raw_frame(rsv_opcode, data);

    // Small frames are never compressed.
    uint32_t pmce_threshold = this->m_parser.pmce_send_no_context_takeover() ? 1024U : 16U;
    if(data.n < pmce_threshold)
      return this->do_ws_send_raw_frame(rsv_opcode, data);

    // Compress the payload and send it. When context takeover is in effect,
    // compressed frames have dependency on each other, so the mutex must not be
    // unlocked before the message is sent completely.
    plain_mutex::unique_lock lock;
    auto& out_buf = this->m_pmce_opt->deflate_output_buffer(lock);
    out_buf.clear();

    if(this->m_parser.pmce_send_no_context_takeover())
      this->m_pmce_opt->deflate_reset(lock);

    bool succ = false;
    try {
      this->m_pmce_opt->deflate_message_stream(lock, data);
      this->m_pmce_opt->deflate_message_finish(lock);

      // RSV1 + opcode
      this->do_ws_send_raw_frame(0b01000000 | rsv_opcode, out_buf);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR(("Could not compress message: $1"), stdex);
      this->m_pmce_opt->deflate_reset(lock);
    }
    return succ;
  }

bool
WS_Server_Session::
ws_send_text(chars_proxy data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // FIN + TEXT
    return this->do_ws_send_raw_data_frame(0b10000000 | 1, data);
  }

bool
WS_Server_Session::
ws_send_binary(chars_proxy data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // FIN + BINARY
    return this->do_ws_send_raw_data_frame(0b10000000 | 2, data);
  }

bool
WS_Server_Session::
ws_ping(chars_proxy data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // The length of the payload of a control frame cannot exceed 125 bytes, so
    // the data string has to be truncated if it's too long.
    chars_proxy ctl_data(data.p, min(data.n, 125));

    // FIN + PING
    return this->do_ws_send_raw_frame(0b10000000 | 9, ctl_data);
  }

bool
WS_Server_Session::
ws_shut_down(uint16_t status, chars_proxy reason) noexcept
  {
    if(!this->do_has_upgraded())
      return this->tcp_shut_down();

    // Compose a CLOSE frame. The length of the payload of a control frame cannot
    // exceed 125 bytes, so the reason string has to be truncated if it's too long.
    static_vector<char, 125> ctl_data;
    ctl_data.push_back(static_cast<char>(status >> 8));
    ctl_data.push_back(static_cast<char>(status));
    ctl_data.append(reason.p, reason.p + min(reason.n, 123));

    bool succ = false;
    try {
      // FIN + CLOSE
      succ = this->do_ws_send_raw_frame(0b10000000 | 8, ctl_data);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Failed to send WebSocket CLOSE notification: $1",
          "[WebSocket server session `$1` (class `$2`)]"),
          stdex);
    }
    succ |= this->tcp_shut_down();
    return succ;
  }

}  // namespace poseidon
