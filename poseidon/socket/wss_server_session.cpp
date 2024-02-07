// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "wss_server_session.hpp"
#include "enums.hpp"
#include "../easy/enums.hpp"
#include "../http/websocket_deflator.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

WSS_Server_Session::
WSS_Server_Session()
  {
  }

WSS_Server_Session::
~WSS_Server_Session()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
void
WSS_Server_Session::
do_call_on_wss_close_once(uint16_t status, chars_view reason)
  {
    if(this->m_closure_notified)
      return;

    this->m_closure_notified = true;
    this->do_on_wss_close(status, reason);
    this->wss_shut_down(1000, "");
  }

void
WSS_Server_Session::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_DEBUG(("Closing WebSocket connection from `$1`: ${errno:full}"), this->remote_address());
    this->do_call_on_wss_close_once(1006, "no CLOSE frame received");
  }

HTTP_Payload_Type
WSS_Server_Session::
do_on_https_request_headers(HTTP_Request_Headers& req, bool close_after_payload)
  {
    if(req.is_proxy) {
      // Reject proxy requests.
      this->do_on_https_request_error(HTTP_STATUS_FORBIDDEN);
      return http_payload_normal;
    }

    if(xstreq(req.method, "OPTIONS")) {
      // Response with allowed methods, or CORS options.
      bool is_cors = any_of(req.headers,
        [](const auto& hpair) {
          return (hpair.first.size() >= 15)
                 && ::rocket::ascii_ci_equal(hpair.first.data(), 15, "Access-Control-", 15);
        });

      HTTP_Response_Headers resp;
      resp.status = 204;
      resp.headers.reserve(8);
      resp.headers.emplace_back(&"Date", system_clock::now());

      if(is_cors) {
        resp.headers.emplace_back(&"Access-Control-Allow-Origin", &"*");
        resp.headers.emplace_back(&"Access-Control-Allow-Methods", &"GET");

        // Allow all headers in RFC 6455.
        resp.headers.emplace_back(&"Access-Control-Allow-Headers",
                        &"Upgrade, Origin, Sec-WebSocket-Version, Sec-WebSocket-Key, "
                         "Sec-WebSocket-Extensions, Sec-WebSocket-Protocol");
      }
      else
        resp.headers.emplace_back(&"Allow", &"GET");

      if(close_after_payload)
        resp.headers.emplace_back(&"Connection", &"close");

      this->https_response_headers_only(move(resp));
      return http_payload_normal;
    }

    // Send the handshake response.
    HTTP_Response_Headers resp;
    this->m_parser.accept_handshake_request(resp, req);
    this->https_response_headers_only(move(resp));

    if(close_after_payload || !this->m_parser.is_server_mode()) {
      // The handshake failed.
      this->do_call_on_wss_close_once(1002, this->m_parser.error_description());
      return http_payload_normal;
    }

    // Initialize extensions.
    if(this->m_parser.pmce_send_max_window_bits() != 0)
      this->m_pmce_opt = new_sh<WebSocket_Deflator>(this->m_parser);

    // Rebuild the URI.
    // XXX: `req.uri_port` and `req.uri_userinfo` are discarded.
    tinyfmt_str uri_fmt;
    uri_fmt << req.uri_host << req.uri_path;

    if(!req.uri_query.empty())
      uri_fmt << '?' << req.uri_query;

    this->do_on_wss_accepted(uri_fmt.extract_string());
    return http_payload_normal;
  }

void
WSS_Server_Session::
do_on_https_request_payload_stream(linear_buffer& data)
  {
    data.clear();
  }

void
WSS_Server_Session::
do_on_https_request_finish(HTTP_Request_Headers&& /*req*/, linear_buffer&& /*data*/, bool /*close_now*/)
  {
  }

void
WSS_Server_Session::
do_on_https_request_error(uint32_t status)
  {
    // This error can be reported synchronously.
    HTTP_Response_Headers resp;
    resp.status = status;
    resp.headers.emplace_back(&"Connection", &"close");
    this->https_response(move(resp), "");

    // Close the connection.
    this->do_call_on_wss_close_once(1002, "handshake failed");
  }

void
WSS_Server_Session::
do_on_https_upgraded_stream(linear_buffer& data, bool eof)
  {
    for(;;) {
      // If something has gone wrong, ignore further incoming data.
      if(this->m_parser.error() || this->m_closure_notified) {
        data.clear();
        return;
      }

      if(!this->m_parser.frame_header_complete()) {
        this->m_parser.parse_frame_header_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_wss_close_once(1002, this->m_parser.error_description());
          return;
        }

        if(!this->m_parser.frame_header_complete())
          return;
      }

      if(!this->m_parser.frame_payload_complete()) {
        this->m_parser.parse_frame_payload_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_wss_close_once(1002, this->m_parser.error_description());
          return;
        }

        auto& payload = this->m_parser.mut_frame_payload();

        if(this->m_parser.frame_header().opcode <= 7) {
          // This is a data frame. If it is not a CONTINUATION, it must start a
          // new message.
          if(this->m_parser.frame_header().opcode != 0) {
            if(this->m_pmce_opt) {
              plain_mutex::unique_lock lock;
              auto& out_buf = this->m_pmce_opt->inflate_output_buffer(lock);
              out_buf.clear();
            }

            this->m_msg.clear();
          }

          // The RSV1 bit indicates part of a compressed message.
          if(this->m_parser.message_rsv1()) {
            if(!this->m_pmce_opt) {
              data.clear();
              this->do_call_on_wss_close_once(1007, "permessage-deflate not initialized properly");
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
          auto opcode = static_cast<WebSocket_OpCode>(this->m_parser.message_opcode());
          ROCKET_ASSERT(is_any_of(opcode, { websocket_text, websocket_binary }));
          this->do_on_wss_message_data_stream(opcode, splice_buffers(this->m_msg, move(payload)));
        }

        if(!this->m_parser.frame_payload_complete())
          return;

        // Handle this frame. Fragmented data frames have already been handled, so
        // don't do it again. Control frames must be processed as a whole.
        if(this->m_parser.frame_header().fin)
          switch(this->m_parser.frame_header().opcode) {
            case 0:  // CONTINUATION
            case 1:  // TEXT
            case 2:  // BINARY
            {
              auto opcode = static_cast<WebSocket_OpCode>(this->m_parser.message_opcode());
              ROCKET_ASSERT(is_any_of(opcode, { websocket_text, websocket_binary }));
              this->do_on_wss_message_finish(opcode, move(this->m_msg));
              break;
            }

            case 8:  // CLOSE
            {
              ROCKET_ASSERT(this->m_closure_notified == false);
              this->m_closure_notified = true;
              data.clear();
              uint16_t bestatus = ROCKET_HTOBE16(1005);
              payload.getn(reinterpret_cast<char*>(&bestatus), 2);
              this->do_on_wss_close(ROCKET_BETOH16(bestatus), payload);
              return;
            }

            case 9:  // PING
            {
              POSEIDON_LOG_TRACE(("WebSocket PING from `$1`: $2"), this->remote_address(), payload);
              this->do_on_wss_message_finish(websocket_ping, move(payload));

              // FIN + PONG
              this->do_wss_send_raw_frame(0b10001010, payload);
              break;
            }

            case 10:  // PONG
            {
              POSEIDON_LOG_TRACE(("WebSocket PONG from `$1`: $2"), this->remote_address(), payload);
              this->do_on_wss_message_finish(websocket_pong, move(payload));
              break;
            }
          }
      }

      this->m_parser.next_frame();
      POSEIDON_LOG_TRACE(("WebSocket parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

void
WSS_Server_Session::
do_on_wss_accepted(cow_string&& caddr)
  {
    POSEIDON_LOG_DEBUG(("Accepted WebSocket from `$1`: $2"), this->remote_address(), caddr);
  }

void
WSS_Server_Session::
do_on_wss_message_data_stream(WebSocket_OpCode /*opcode*/, linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_wss_message_finish()`, but
    // perform some security checks, so we won't be affected by compromized
    // 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_websocket_message_length = 1048576;

    auto conf_value = conf_file.query("network", "http", "max_websocket_message_length");
    if(conf_value.is_integer())
      max_websocket_message_length = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.http.max_websocket_message_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if(max_websocket_message_length < 0)
      POSEIDON_THROW((
          "`network.http.max_websocket_message_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_websocket_message_length, conf_file.path());

    if(data.size() > (uint64_t) max_websocket_message_length)
      POSEIDON_THROW((
          "WebSocket text data message too large: `$3` > `$4`",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_websocket_message_length);
  }

void
WSS_Server_Session::
do_on_wss_close(uint16_t status, chars_view reason)
  {
    POSEIDON_LOG_DEBUG(("WebSocket CLOSE from `$1` (status $2): $3"), this->remote_address(), status, reason);
  }

bool
WSS_Server_Session::
do_wss_send_raw_frame(int rsv_opcode, chars_view data)
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
    return this->ssl_send(fmt);
  }

bool
WSS_Server_Session::
wss_send(WebSocket_OpCode opcode, chars_view data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this));

    switch(opcode) {
      case 1:  // TEXT
      case 2:  // BINARY
        // Check whether the message should be compressed. We believe that small
        // (including empty) messages are not worth compressing.
        if(this->m_pmce_opt) {
          uint32_t pmce_threshold = this->m_parser.pmce_send_no_context_takeover() * 1024U + 16U;
          if(data.n >= pmce_threshold) {
            // Compress the payload and send it. When context takeover is active,
            // compressed frames have dependency on each other, so the mutex must
            // not be unlocked before the message is sent completely.
            plain_mutex::unique_lock lock;
            auto& out_buf = this->m_pmce_opt->deflate_output_buffer(lock);
            out_buf.clear();

            if(this->m_parser.pmce_send_no_context_takeover())
              this->m_pmce_opt->deflate_reset(lock);

            try {
              this->m_pmce_opt->deflate_message_stream(lock, data);
              this->m_pmce_opt->deflate_message_finish(lock);

              // FIN + RSV1 + opcode
              return this->do_wss_send_raw_frame(0b11000000 | opcode, out_buf);
            }
            catch(exception& stdex) {
              // When an error occurred, the deflator is left in an indeterminate
              // state, so reset it and send the message uncompressed.
              POSEIDON_LOG_ERROR(("Could not compress message: $1"), stdex);
              this->m_pmce_opt->deflate_reset(lock);
            }
          }
        }

        // Send the message uncompressed.
        // FIN + opcode
        return this->do_wss_send_raw_frame(0b10000000 | opcode, data);

      case 9:  // PING
      case 10:  // PONG
        // Control messages can't be fragmented.
        if(data.n > 125)
          POSEIDON_THROW((
              "Control frame too large: `$3` > `125`",
              "[WebSocket server session `$1` (class `$2`)]"),
              this, typeid(*this), data.n);

        // Control messages can't be compressed, so send it as is.
        // FIN + opcode
        return this->do_wss_send_raw_frame(0b10000000 | opcode, data);

      default:
        POSEIDON_THROW((
            "WebSocket opcode `$3` not supported",
            "[WebSocket server session `$1` (class `$2`)]"),
            this, typeid(*this), opcode);
    }
  }

bool
WSS_Server_Session::
wss_shut_down(uint16_t status, chars_view reason) noexcept
  {
    if(!this->do_has_upgraded() || (this->socket_state() >= socket_closing))
      return this->ssl_shut_down();

    bool succ = false;
    try {
      // Compose a CLOSE frame. A control frame shall not be fragmented, so the
      // length of its payload cannot exceed 125 bytes, and the reason string has
      // to be truncated if it's too long.
      static_vector<char, 125> data;
      uint16_t bestatus = ROCKET_HTOBE16(status);
      const char* eptr = reinterpret_cast<const char*>(&bestatus) + 2;
      data.append(eptr - 2, eptr);
      eptr = reason.p + ::std::min<size_t>(reason.n, 123);
      data.append(reason.p, eptr);

      // FIN + CLOSE
      succ = this->do_wss_send_raw_frame(0b10000000 | 8, data);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Failed to send WebSocket CLOSE notification: $1",
          "[WebSocket server session `$1` (class `$2`)]"),
          stdex);
    }
    succ |= this->ssl_shut_down();
    return succ;
  }

bool
WSS_Server_Session::
wss_send(Easy_WS_Event opcode, chars_view data)
  {
    switch(opcode) {
      case easy_ws_text:
        return this->wss_send(websocket_text, data);

      case easy_ws_binary:
        return this->wss_send(websocket_binary, data);

      case easy_ws_ping:
        return this->wss_send(websocket_ping, data);

      case easy_ws_pong:
        return this->wss_send(websocket_pong, data);

      case easy_ws_close:
        return this->wss_shut_down(1000, data);

      default:
        POSEIDON_THROW((
            "Easy WebSocket event code `$3` not supported",
            "[WebSocket client session `$1` (class `$2`)]"),
            this, typeid(*this), opcode);
    }
  }

}  // namespace poseidon
