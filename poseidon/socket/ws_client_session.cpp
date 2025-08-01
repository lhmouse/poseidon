// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "ws_client_session.hpp"
#include "../http/websocket_deflator.hpp"
#include "../utils.hpp"
namespace poseidon {

WS_Client_Session::
WS_Client_Session(const cow_string& raw_path, const cow_string& raw_query)
  {
    HTTP_C_Headers req;
    this->m_parser.create_handshake_request(req);
    req.is_ssl = false;
    req.raw_path = raw_path;
    req.raw_query = raw_query;
    req.set_request_host(*this, this->http_default_host());
    this->do_http_raw_request(move(req), "");
  }

WS_Client_Session::
~WS_Client_Session()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
void
WS_Client_Session::
do_call_on_ws_close_once(WS_Status status, chars_view reason)
  {
    if(this->m_closure_notified)
      return;

    this->m_closure_notified = true;
    this->do_on_ws_close(status, reason);
    this->ws_shut_down(ws_status_normal, "");
  }

void
WS_Client_Session::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_INFO(("Closing WebSocket to `$1`: ${errno:full}"), this->remote_address());
    this->do_call_on_ws_close_once(ws_status_no_close_frame, "no CLOSE frame received");
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
do_on_http_response_finish(HTTP_S_Headers&& resp, linear_buffer&& /*data*/)
  {
    // Accept the handshake response.
    this->m_parser.accept_handshake_response(resp);

    if(!this->m_parser.is_client_mode()) {
      // The handshake failed.
      this->do_call_on_ws_close_once(ws_status_protocol_error, this->m_parser.description());
      return;
    }

    // Initialize extensions.
    if(this->m_parser.pmce_send_window_bits() != 0)
      this->m_pmce_opt = new_sh<WebSocket_Deflator>(this->m_parser);

    this->do_on_ws_connected();
  }

void
WS_Client_Session::
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
          this->do_call_on_ws_close_once(ws_status_protocol_error, this->m_parser.description());
          return;
        }

        if(!this->m_parser.frame_header_complete())
          return;
      }

      if(!this->m_parser.frame_payload_complete()) {
        this->m_parser.parse_frame_payload_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_ws_close_once(ws_status_protocol_error, this->m_parser.description());
          return;
        }

        auto& payload = this->m_parser.mut_frame_payload();

        if(this->m_parser.frame_header().opcode <= 7) {
          // This is a data frame. If it is not a CONTINUATION, it must start a
          // new message.
          if(this->m_parser.frame_header().opcode != 0) {
            this->m_msg.clear();
            if(this->m_pmce_opt) {
              plain_mutex::unique_lock lock;
              auto& out_buf = this->m_pmce_opt->inflate_output_buffer(lock);
              out_buf.clear();
            }
          }

          // The RSV1 bit indicates part of a compressed message.
          if(this->m_parser.message_rsv1()) {
            if(!this->m_pmce_opt) {
              data.clear();
              this->do_call_on_ws_close_once(ws_status_unexpected_error, "PMCE not initialized");
              return;
            }

            plain_mutex::unique_lock lock;
            this->m_pmce_opt->inflate_message_stream(lock, payload,
                                   this->m_parser.max_message_length());

            if(this->m_parser.message_fin())
              this->m_pmce_opt->inflate_message_finish(lock);

            // Replace the payload... nasty.
            auto& out_buf = this->m_pmce_opt->inflate_output_buffer(lock);
            payload.swap(out_buf);
          }

          // If this is a data frame or continuation, its payload is part of a
          // (potentially fragmented) data message, so combine it.
          auto opcode = static_cast<WS_Opcode>(this->m_parser.message_opcode());
          ROCKET_ASSERT(is_any_of(opcode, { ws_TEXT, ws_BINARY }));
          this->do_on_ws_message_data_stream(opcode, splice_buffers(this->m_msg, move(payload)));
        }

        if(!this->m_parser.frame_payload_complete())
          return;

        // Handle this frame. Fragmented data frames have already been handled, so
        // don't do it again. Control frames must be processed as a whole.
        if(this->m_parser.frame_header().fin)
          switch(this->m_parser.frame_header().opcode)
            {
            case ws_CONTINUATION:
            case ws_TEXT:
            case ws_BINARY:
              {
                auto opcode = static_cast<WS_Opcode>(this->m_parser.message_opcode());
                ROCKET_ASSERT(is_any_of(opcode, { ws_TEXT, ws_BINARY }));
                this->do_on_ws_message_finish(opcode, move(this->m_msg));
                break;
              }

            case ws_CLOSE:
              {
                ROCKET_ASSERT(this->m_closure_notified == false);
                this->m_closure_notified = true;
                data.clear();

                // Get the status code in big-endian order.
                char temp[2];
                ROCKET_STORE_BE16(temp, ws_status_no_status_code);
                payload.getn(temp, 2);
                this->do_on_ws_close(static_cast<WS_Status>(ROCKET_LOAD_BE16(temp)), payload);
                return;
              }

            case ws_PING:
              POSEIDON_LOG_TRACE(("PING from `$1`: $2"), this->remote_address(), payload);
              this->do_on_ws_message_finish(ws_PING, move(payload));

              // FIN + PONG
              this->do_ws_send_raw_frame(0b10001010, payload);
              break;

            case ws_PONG:
              POSEIDON_LOG_TRACE(("PONG from `$1`: $2"), this->remote_address(), payload);
              this->do_on_ws_message_finish(ws_PONG, move(payload));
              break;
            }
      }

      this->m_parser.next_frame();
      POSEIDON_LOG_TRACE(("Parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

void
WS_Client_Session::
do_on_ws_connected()
  {
    POSEIDON_LOG_INFO(("Connected WebSocket to `$1`"), this->remote_address());
  }

void
WS_Client_Session::
do_on_ws_message_data_stream(WS_Opcode /*opcode*/, linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_message_finish()`, but
    // perform some security checks, so we won't be affected by compromised
    // 3rd-party servers.
    if(data.size() > this->m_parser.max_message_length())
      POSEIDON_THROW((
          "WebSocket text data message too large: `$3` > `$4`",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), this->m_parser.max_message_length());
  }

void
WS_Client_Session::
do_on_ws_close(WS_Status status, chars_view reason)
  {
    POSEIDON_LOG_INFO(("Closed WebSocket to `$1`: $2: $3"), this->remote_address(), status, reason);
  }

bool
WS_Client_Session::
do_ws_send_raw_frame(int rsv_opcode, chars_view data)
  {
    // Compose a single frame and send it. Frames to servers must be masked.
    WebSocket_Frame_Header header;
    header.fin = rsv_opcode >> 7 & 1;
    header.rsv1 = rsv_opcode >> 6 & 1;
    header.rsv2 = rsv_opcode >> 5 & 1;
    header.rsv3 = rsv_opcode >> 4 & 1;
    header.opcode = static_cast<WS_Opcode>(rsv_opcode & 15);
    header.masked = 1;
    header.masking_key = static_cast<uint32_t>(0x80 | ::random());
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
ws_send(WS_Opcode opcode, chars_view data)
  {
    switch(static_cast<uint32_t>(opcode))
      {
      case ws_TEXT:
      case ws_BINARY:
        {
          if(this->do_has_upgraded() && this->m_pmce_opt) {
            // Don't compress small frames when it's not worth the cost.
            // TODO: Is this configurable?
            size_t compression_threshold = 64;
            if(this->m_parser.pmce_send_no_context_takeover())
              compression_threshold = 1024;

            if(data.n >= compression_threshold) {
              // Compress the payload and send it. When context takeover is
              // active, compressed frames have dependency on each other, so
              // the mutex must not be unlocked before the message is sent
              // completely.
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

      case ws_PING:
      case ws_PONG:
        {
          if(data.n > 125)
            POSEIDON_THROW((
                "Control frame too large: `$3` > `125`",
                "[WebSocket client session `$1` (class `$2`)]"),
                this, typeid(*this), data.n);

          // Control messages can't be compressed, so send it as is.
          // FIN + opcode
          return this->do_ws_send_raw_frame(0b10000000 | opcode, data);
        }

      default:
        POSEIDON_THROW((
            "WebSocket opcode `$3` not supported",
            "[WebSocket client session `$1` (class `$2`)]"),
            this, typeid(*this), opcode);
      }
  }

bool
WS_Client_Session::
ws_shut_down(WS_Status status, chars_view reason)
  noexcept
  {
    if(!this->do_has_upgraded() || (this->socket_state() >= socket_closing))
      return this->tcp_shut_down();

    bool succ = false;
    try {
      // Compose a CLOSE frame. A control frame shall not be fragmented, so the
      // length of its payload cannot exceed 125 bytes, and the reason string has
      // to be truncated if it's too long.
      static_vector<char, 125> data;
      data.resize(2);
      ROCKET_STORE_BE16(data.mut_data(), status);
      data.append(reason.p, reason.p + min(reason.n, data.capacity() - 2));

      // FIN + CLOSE
      succ = this->do_ws_send_raw_frame(0b10000000 | 8, data);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Failed to send WebSocket CLOSE notification: $3",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this), stdex);
    }
    succ |= this->tcp_shut_down();
    return succ;
  }

bool
WS_Client_Session::
ws_shut_down(int status, chars_view reason)
  noexcept
  {
    WS_Status real_status = ws_status_policy_violation;
    if((status >= 1000) && (status <= 4999))
      real_status = static_cast<WS_Status>(real_status);
    return this->ws_shut_down(real_status, reason);
  }

}  // namespace poseidon
