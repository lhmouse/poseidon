// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "wss_client_session.hpp"
#include "../http/websocket_deflator.hpp"
#include "../utils.hpp"
namespace poseidon {

WSS_Client_Session::
WSS_Client_Session(const cow_string& path, const cow_string& query)
  {
    if(!path.starts_with("/"))
      POSEIDON_THROW(("Absolute path `$1` not allowed"), path);

    this->m_path = path;
    this->m_query = query;
  }

WSS_Client_Session::
~WSS_Client_Session()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
void
WSS_Client_Session::
do_call_on_wss_close_once(WebSocket_Status status, chars_view reason)
  {
    if(this->m_closure_notified)
      return;

    this->m_closure_notified = true;
    this->do_on_wss_close(status, reason);
    this->wss_shut_down(websocket_status_normal_closure, "");
  }

void
WSS_Client_Session::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_DEBUG(("Closing WebSocket connection to `$1`: ${errno:full}"), this->remote_address());
    this->do_call_on_wss_close_once(websocket_status_no_close_frame, "no CLOSE frame received");
  }

void
WSS_Client_Session::
do_on_ssl_connected()
  {
    // Send a handshake request.
    HTTP_Request_Headers req;
    this->m_parser.create_handshake_request(req);
    req.is_ssl = true;
    req.uri_path = this->m_path;
    req.uri_query = this->m_query;
    this->https_request(move(req), "");
  }

void
WSS_Client_Session::
do_on_https_response_payload_stream(linear_buffer& data)
  {
    // The request payload is ignored.
    data.clear();
  }

void
WSS_Client_Session::
do_on_https_response_finish(HTTP_Response_Headers&& resp, linear_buffer&& /*data*/, bool close_now)
  {
    // Accept the handshake response.
    this->m_parser.accept_handshake_response(resp);

    if(close_now || !this->m_parser.is_client_mode()) {
      // The handshake failed.
      this->do_call_on_wss_close_once(websocket_status_protocol_error, this->m_parser.error_description());
      return;
    }

    // Initialize extensions.
    if(this->m_parser.pmce_send_window_bits() != 0)
      this->m_pmce_opt = new_sh<WebSocket_Deflator>(this->m_parser);

    // Rebuild the URI.
    this->do_on_wss_connected(this->https_default_host() + this->m_path + '?' + this->m_query);
  }

void
WSS_Client_Session::
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
          this->do_call_on_wss_close_once(websocket_status_protocol_error, this->m_parser.error_description());
          return;
        }

        if(!this->m_parser.frame_header_complete())
          return;
      }

      if(!this->m_parser.frame_payload_complete()) {
        this->m_parser.parse_frame_payload_from_stream(data);

        if(this->m_parser.error()) {
          data.clear();
          this->do_call_on_wss_close_once(websocket_status_protocol_error, this->m_parser.error_description());
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
              this->do_call_on_wss_close_once(websocket_status_unexpected_error, "PMCE not initialized");
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
          auto opcode = static_cast<WebSocket_Opcode>(this->m_parser.message_opcode());
          ROCKET_ASSERT(is_any_of(opcode, { websocket_TEXT, websocket_BINARY }));
          this->do_on_wss_message_data_stream(opcode, splice_buffers(this->m_msg, move(payload)));
        }

        if(!this->m_parser.frame_payload_complete())
          return;

        // Handle this frame. Fragmented data frames have already been handled, so
        // don't do it again. Control frames must be processed as a whole.
        if(this->m_parser.frame_header().fin)
          switch(this->m_parser.frame_header().opcode)
            {
            case 0:  // CONTINUATION
            case 1:  // TEXT
            case 2:  // BINARY
              {
                auto opcode = static_cast<WebSocket_Opcode>(this->m_parser.message_opcode());
                ROCKET_ASSERT(is_any_of(opcode, { websocket_TEXT, websocket_BINARY }));
                this->do_on_wss_message_finish(opcode, move(this->m_msg));
              }
              break;

            case 8:  // CLOSE
              {
                ROCKET_ASSERT(this->m_closure_notified == false);
                this->m_closure_notified = true;
                data.clear();

                // Get the status code in big-endian order..
                uint16_t bestatus;
                WebSocket_Status status = websocket_status_no_status_code;
                if(payload.getn(reinterpret_cast<char*>(&bestatus), 2) >= 2)
                  status = static_cast<WebSocket_Status>(ROCKET_BETOH16(bestatus));
                this->do_on_wss_close(status, payload);
              }
              return;

            case 9:  // PING
              {
                POSEIDON_LOG_TRACE(("WebSocket PING from `$1`: $2"), this->remote_address(), payload);
                this->do_on_wss_message_finish(websocket_PING, move(payload));

                // FIN + PONG
                this->do_wss_send_raw_frame(0b10001010, payload);
              }
              break;

            case 10:  // PONG
              {
                POSEIDON_LOG_TRACE(("WebSocket PONG from `$1`: $2"), this->remote_address(), payload);
                this->do_on_wss_message_finish(websocket_PONG, move(payload));
              }
              break;
            }
      }

      this->m_parser.next_frame();
      POSEIDON_LOG_TRACE(("WebSocket parser done: data.size = $1, eof = $2"), data.size(), eof);
    }
  }

void
WSS_Client_Session::
do_on_wss_connected(cow_string&& caddr)
  {
    POSEIDON_LOG_DEBUG(("Connected WebSocket to `$1`: $2"), this->remote_address(), caddr);
  }

void
WSS_Client_Session::
do_on_wss_message_data_stream(WebSocket_Opcode /*opcode*/, linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_wss_message_finish()`, but
    // perform some security checks, so we won't be affected by compromised
    // 3rd-party servers.
    if(data.size() > this->m_parser.max_message_length())
      POSEIDON_THROW((
          "WebSocket text data message too large: `$3` > `$4`",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), this->m_parser.max_message_length());
  }

void
WSS_Client_Session::
do_on_wss_close(WebSocket_Status status, chars_view reason)
  {
    POSEIDON_LOG_DEBUG(("WebSocket CLOSE from `$1` (status $2): $3"),
                        this->remote_address(), status, reason);
  }

bool
WSS_Client_Session::
do_wss_send_raw_frame(int rsv_opcode, chars_view data)
  {
    // Compose a single frame and send it. Frames to servers must be masked.
    WebSocket_Frame_Header header;
    header.fin = rsv_opcode >> 7 & 1;
    header.rsv1 = rsv_opcode >> 6 & 1;
    header.rsv2 = rsv_opcode >> 5 & 1;
    header.rsv3 = rsv_opcode >> 4 & 1;
    header.opcode = static_cast<WebSocket_Opcode>(rsv_opcode & 15);
    header.masked = 1;
    header.masking_key = static_cast<uint32_t>(0x80 | ::random());
    header.payload_len = data.n;

    tinyfmt_ln fmt;
    header.encode(fmt);
    fmt.putn(data.p, data.n);
    auto buf = fmt.extract_buffer();
    header.mask_payload(buf.mut_end() - data.n, data.n);
    return this->ssl_send(buf);
  }

bool
WSS_Client_Session::
wss_send(WebSocket_Opcode opcode, chars_view data)
  {
    if(!this->do_has_upgraded())
      POSEIDON_THROW((
          "WebSocket handshake not complete yet",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this));

    switch(static_cast<uint32_t>(opcode))
      {
      case websocket_TEXT:
      case websocket_BINARY:
        {
          if(this->m_pmce_opt && (data.n >= this->m_parser.pmce_threshold())) {
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
              return this->do_wss_send_raw_frame(0b11000000 | opcode, out_buf);
            }
            catch(exception& stdex) {
              // When an error occurred, the deflator is left in an indeterminate
              // state, so reset it and send the message uncompressed.
              POSEIDON_LOG_ERROR(("Could not compress message: $1"), stdex);
              this->m_pmce_opt->deflate_reset(lock);
            }
          }

          // Send the message uncompressed.
          // FIN + opcode
          return this->do_wss_send_raw_frame(0b10000000 | opcode, data);
        }

      case websocket_PING:
      case websocket_PONG:
        {
          // Control messages can't be fragmented.
          if(data.n > 125)
            POSEIDON_THROW((
                "Control frame too large: `$3` > `125`",
                "[WebSocket client session `$1` (class `$2`)]"),
                this, typeid(*this), data.n);

          // Control messages can't be compressed, so send it as is.
          // FIN + opcode
          return this->do_wss_send_raw_frame(0b10000000 | opcode, data);
        }

      default:
        POSEIDON_THROW((
            "WebSocket opcode `$3` not supported",
            "[WebSocket client session `$1` (class `$2`)]"),
            this, typeid(*this), opcode);
      }
  }

bool
WSS_Client_Session::
wss_shut_down(WebSocket_Status status, chars_view reason) noexcept
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
          "Failed to send WebSocket CLOSE notification: $3",
          "[WebSocket client session `$1` (class `$2`)]"),
          this, typeid(*this), stdex);
    }
    succ |= this->ssl_shut_down();
    return succ;
  }

}  // namespace poseidon
