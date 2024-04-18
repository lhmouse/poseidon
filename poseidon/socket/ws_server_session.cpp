// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "ws_server_session.hpp"
#include "../http/websocket_deflator.hpp"
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

POSEIDON_VISIBILITY_HIDDEN
void
WS_Server_Session::
do_call_on_ws_close_once(WebSocket_Status status, chars_view reason)
  {
    if(this->m_closure_notified)
      return;

    this->m_closure_notified = true;
    this->do_on_ws_close(status, reason);
    this->ws_shut_down(websocket_status_normal_closure, "");
  }

void
WS_Server_Session::
do_abstract_socket_on_closed()
  {
    if(!this->do_has_upgraded())
      return;

    POSEIDON_LOG_DEBUG(("Closing WebSocket connection from `$1`: ${errno:full}"), this->remote_address());
    this->do_call_on_ws_close_once(websocket_status_no_close_frame, "no CLOSE frame received");
  }

HTTP_Payload_Type
WS_Server_Session::
do_on_http_request_headers(HTTP_Request_Headers& req, bool close_after_payload)
  {
    this->do_ws_complete_handshake(req, close_after_payload);
    return http_payload_normal;
  }

void
WS_Server_Session::
do_on_http_request_payload_stream(linear_buffer& data)
  {
    data.clear();
  }

void
WS_Server_Session::
do_on_http_request_finish(HTTP_Request_Headers&& /*req*/, linear_buffer&& /*data*/, bool /*close_now*/)
  {
  }

void
WS_Server_Session::
do_on_http_request_error(HTTP_Status status)
  {
    // This error can be reported synchronously.
    HTTP_Response_Headers resp;
    resp.status = status;
    resp.headers.emplace_back(&"Connection", &"close");
    this->http_response(move(resp), "");

    // Close the connection.
    this->do_call_on_ws_close_once(websocket_status_no_close_frame, "handshake rejected by HTTP error");
  }

void
WS_Server_Session::
do_on_http_upgraded_stream(linear_buffer& data, bool eof)
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
          this->do_call_on_ws_close_once(websocket_status_protocol_error, this->m_parser.error_description());
          return;
        }

        if(!this->m_parser.frame_header_complete())
          return;
      }

      if(!this->m_parser.frame_payload_complete()) {
        this->m_parser.parse_frame_payload_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_ws_close_once(websocket_status_protocol_error, this->m_parser.error_description());
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
              this->do_call_on_ws_close_once(websocket_status_unexpected_error, "PMCE not initialized");
              return;
            }

            plain_mutex::unique_lock lock;
            this->m_pmce_opt->inflate_message_stream(lock, payload, this->m_parser.max_message_length());

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
          this->do_on_ws_message_data_stream(opcode, splice_buffers(this->m_msg, move(payload)));
        }

        if(!this->m_parser.frame_payload_complete())
          return;

        // Handle this frame. Fragmented data frames have already been handled, so
        // don't do it again. Control frames must be processed as a whole.
        if(this->m_parser.frame_header().fin)
          switch(this->m_parser.frame_header().opcode)
            {
            case websocket_continuation:
            case websocket_text:
            case websocket_binary:
              {
                auto opcode = static_cast<WebSocket_OpCode>(this->m_parser.message_opcode());
                ROCKET_ASSERT(is_any_of(opcode, { websocket_text, websocket_binary }));
                this->do_on_ws_message_finish(opcode, move(this->m_msg));
              }
              break;

            case websocket_close:
              {
                ROCKET_ASSERT(this->m_closure_notified == false);
                this->m_closure_notified = true;
                data.clear();

                // Get the status code in big-endian order..
                uint16_t bestatus;
                if(payload.getn(reinterpret_cast<char*>(&bestatus), 2) >= 2)
                  this->do_on_ws_close(static_cast<WebSocket_Status>(ROCKET_BETOH16(bestatus)), payload);
                else
                  this->do_on_ws_close(websocket_status_no_status_code, payload);
              }
              return;

            case websocket_ping:
              {
                POSEIDON_LOG_TRACE(("WebSocket PING from `$1`: $2"), this->remote_address(), payload);
                this->do_on_ws_message_finish(websocket_ping, move(payload));

                // FIN + PONG
                this->do_ws_send_raw_frame(0b10001010, payload);
              }
              break;

            case websocket_pong:
              {
                POSEIDON_LOG_TRACE(("WebSocket PONG from `$1`: $2"), this->remote_address(), payload);
                this->do_on_ws_message_finish(websocket_pong, move(payload));
              }
              break;
            }
      }

      this->m_parser.next_frame();
      POSEIDON_LOG_TRACE(("WebSocket parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

void
WS_Server_Session::
do_on_ws_accepted(cow_string&& caddr)
  {
    POSEIDON_LOG_DEBUG(("Accepted WebSocket from `$1`: $2"), this->remote_address(), caddr);
  }

void
WS_Server_Session::
do_on_ws_message_data_stream(WebSocket_OpCode /*opcode*/, linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_message_finish()`, but
    // perform some security checks, so we won't be affected by compromised
    // 3rd-party servers.
    if(data.size() > this->m_parser.max_message_length())
      POSEIDON_THROW((
          "WebSocket text data message too large: `$3` > `$4`",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), this->m_parser.max_message_length());
  }

void
WS_Server_Session::
do_on_ws_close(WebSocket_Status status, chars_view reason)
  {
    POSEIDON_LOG_DEBUG(("WebSocket CLOSE from `$1` (status $2): $3"),
                       this->remote_address(), status, reason);
  }

void
WS_Server_Session::
do_ws_complete_handshake(HTTP_Request_Headers& req, bool close_after_payload)
  {
    if(req.is_proxy) {
      // Reject proxy requests.
      this->do_on_http_request_error(http_status_forbidden);
      return;
    }

    // Send the handshake response.
    HTTP_Response_Headers resp;
    this->m_parser.accept_handshake_request(resp, req);
    this->http_response_headers_only(move(resp));

    if(req.method == http_method_OPTIONS)
      return;

    if(close_after_payload || !this->m_parser.is_server_mode()) {
      // The handshake failed.
      this->do_call_on_ws_close_once(websocket_status_protocol_error, this->m_parser.error_description());
      return;
    }

    // Initialize extensions.
    if(this->m_parser.pmce_send_max_window_bits() != 0)
      this->m_pmce_opt = new_sh<WebSocket_Deflator>(this->m_parser);

    // Rebuild the URI.
    this->do_on_ws_accepted(req.uri_host + req.uri_path + '?' + req.uri_query);
  }

bool
WS_Server_Session::
do_ws_send_raw_frame(int rsv_opcode, chars_view data)
  {
    // Compose a single frame and send it. Frames to clients will not be masked.
    WebSocket_Frame_Header header;
    header.fin = rsv_opcode >> 7 & 1;
    header.rsv1 = rsv_opcode >> 6 & 1;
    header.rsv2 = rsv_opcode >> 5 & 1;
    header.rsv3 = rsv_opcode >> 4 & 1;
    header.opcode = static_cast<WebSocket_OpCode>(rsv_opcode & 15);
    header.payload_len = data.n;

    tinyfmt_ln fmt;
    header.encode(fmt);
    fmt.putn(data.p, data.n);
    return this->tcp_send(fmt);
  }

bool
WS_Server_Session::
ws_send(WebSocket_OpCode opcode, chars_view data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this));

    switch(static_cast<uint32_t>(opcode))
      {
      case websocket_text:
      case websocket_binary:
        {
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
                return this->do_ws_send_raw_frame(0b11000000 | opcode, out_buf);
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
          return this->do_ws_send_raw_frame(0b10000000 | opcode, data);
        }

      case websocket_ping:
      case websocket_pong:
        {
          if(data.n > 125)
            POSEIDON_THROW((
                "Control frame too large: `$3` > `125`",
                "[WebSocket server session `$1` (class `$2`)]"),
                this, typeid(*this), data.n);

          // Control messages can't be compressed, so send it as is.
          // FIN + opcode
          return this->do_ws_send_raw_frame(0b10000000 | opcode, data);
        }

      default:
        POSEIDON_THROW((
            "WebSocket opcode `$3` not supported",
            "[WebSocket server session `$1` (class `$2`)]"),
            this, typeid(*this), opcode);
      }
  }

bool
WS_Server_Session::
ws_shut_down(WebSocket_Status status, chars_view reason) noexcept
  {
    if(!this->do_has_upgraded() || (this->socket_state() >= socket_closing))
      return this->tcp_shut_down();

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
      succ = this->do_ws_send_raw_frame(0b10000000 | 8, data);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Failed to send WebSocket CLOSE notification: $3",
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this), stdex);
    }
    succ |= this->tcp_shut_down();
    return succ;
  }

}  // namespace poseidon
