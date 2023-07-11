// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "ws_server_session.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

WS_Server_Session::
WS_Server_Session(unique_posix_fd&& fd)
  : HTTP_Server_Session(::std::move(fd))  // server constructor
  {
  }

WS_Server_Session::
~WS_Server_Session()
  {
  }

void
WS_Server_Session::
do_call_on_ws_close_once(uint16_t status, const char* reason)
  {
    if(this->m_closure_notified)
      return;

    this->m_closure_notified = true;
    this->do_on_ws_close(status, reason);
    this->quick_close();
  }

void
WS_Server_Session::
do_abstract_socket_on_closed()
  {
    this->do_call_on_ws_close_once(1005, "");
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

// TODO remove this
for(size_t hindex = 0; hindex < resp.headers.size(); ++ hindex)
  if(resp.headers.at(hindex).first == "Sec-WebSocket-Extensions")
    resp.headers.erase(hindex --);
// ^^^^ remove this

    this->http_response(::std::move(resp), "", 0);

    // If the response is a failure, close the connection.
    if((resp.status != 101) || close_now)
      this->tcp_close();
  }

void
WS_Server_Session::
do_on_http_upgraded_stream(linear_buffer& data, bool /*eof*/)
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

            if(this->m_parser.message_opcode() == 1) {
              // TEXT
              this->do_on_ws_text_stream(this->m_msg);

              if(this->m_parser.message_fin())
                this->do_on_ws_text(::std::move(this->m_msg));
            }
            else {
              // BINARY
              this->do_on_ws_binary_stream(this->m_msg);

              if(this->m_parser.message_fin())
                this->do_on_ws_binary(::std::move(this->m_msg));
            }
            break;
          }

          case 8: {  // close
            uint16_t bestatus = htobe16(1005);
            const char* reason = "";

            if(payload.size() >= 2U) {
              // Get the status code and description string that were sent from
              // the client.
              ::memcpy(&bestatus, payload.data(), 2U);
              payload.discard(2U);
              payload.putc(0);
              reason = payload.data();
            }

            this->do_call_on_ws_close_once(be16toh(bestatus), reason);
            break;
          }

          case 9:  // ping
            POSEIDON_LOG_TRACE(("WebSocket PING from `$1`: $2"), this->remote_address(), payload);
            this->do_ws_send_raw_frame(10, payload.data(), payload.size());
            break;

          case 10:  // pong
            POSEIDON_LOG_TRACE(("WebSocket PONG from `$1`: $2"), this->remote_address(), payload);
            this->do_on_ws_pong(::std::move(payload));
            break;
        }
      }
    }
  }

void
WS_Server_Session::
do_on_ws_text_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_binary_stream()`, but
    // perform some security checks, so we won't be affected by compromized
    // 3rd-party servers.
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
          "[WebSocket server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_websocket_text_message_length);
  }

void
WS_Server_Session::
do_on_ws_binary_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_ws_binary_stream()`, but
    // perform some security checks, so we won't be affected by compromized
    // 3rd-party servers.
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
do_on_ws_close(uint16_t status, const char* reason)
  {
    // Send a CLOSE frame to the client. The connection will be closed after
    // this function returns, so there is no need to close it here.
    char data[128];
    uint16_t bestatus = htobe16(status);
    ::memcpy(data, &bestatus, 2U);

    size_t size = 2U +  min(::strlen(reason), 123U);
    ::memcpy(data + 2U, reason, size - 2U);

    this->do_ws_send_raw_frame(8, data, size);
  }

bool
WS_Server_Session::
do_ws_send_raw_frame(uint8_t opcode, char_sequence data)
  {
    // Compose a single frame and send it. Server-to-client frames will not be
    // masked.
    WebSocket_Frame_Header header;
    header.fin = 1;
    header.opcode = opcode & 15;

    tinyfmt_ln fmt;
    header.encode(fmt);
    fmt.putn(data.p, data.n);
    return this->tcp_send(fmt.data(), fmt.size());
  }

bool
WS_Server_Session::
ws_send_text(char_sequence data)
  {
    // TEXT := opcode 1
    return this->do_ws_send_raw_frame(1, data, size);
  }

bool
WS_Server_Session::
ws_send_binary(char_sequence data)
  {
    // BINARY := opcode 2
    return this->do_ws_send_raw_frame(2, data, size);
  }

bool
WS_Server_Session::
ws_ping(char_sequence data)
  {
    // PING := opcode 9
    return this->do_ws_send_raw_frame(9, data, min(size, 125U));
  }

bool
WS_Server_Session::
ws_close(uint16_t status, const char* reason_opt)
  {
    // Write the status code in big-endian order.
    char data[128];
    uint16_t bestatus = htobe16(status);
    ::memcpy(data, &bestatus, 2U);

    size_t size = 2U;
    if(reason_opt) {
      // The total length of the payload of a control frame cannot exceed 125
      // bytes, so the reason string has to be truncated silently if it's too
      // long.
      size += min(::strlen(reason_opt), 123U);
      ::memcpy(data + 2U, reason_opt, size - 2U);
    }

    // CLOSE := opcode 9
    bool succ = this->do_ws_send_raw_frame(8, data, size);
    succ |= this->tcp_close();
    return succ;
  }

}  // namespace poseidon
