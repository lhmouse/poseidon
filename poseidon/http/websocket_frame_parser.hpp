// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_FRAME_PARSER_
#define POSEIDON_HTTP_WEBSOCKET_FRAME_PARSER_

#include "../fwd.hpp"
#include "websocket_frame_header.hpp"
namespace poseidon {

class WebSocket_Frame_Parser
  {
  private:
    int m_default_compression_level = 6;
    uint32_t m_max_message_length = 1048576;

    WebSocket_Frame_Header m_frm_header;
    linear_buffer m_frm_payload;
    uint64_t m_frm_payload_rem = 0;
    const char* m_error_desc = nullptr;

    enum WSHS_State : uint8_t
      {
        wshs_pending     = 0,
        wshs_c_req_sent  = 1,
        wshs_s_accepted  = 2,
        wshs_c_accepted  = 3,
      };

    enum WSF_State : uint8_t
      {
        wsf_new           = 0,
        wsf_header_done   = 1,
        wsf_payload_done  = 2,
        wsf_error         = 9,
      };

    union {
      uint32_t m_state_stor = 0;

      struct {
        // 0
        WSHS_State m_wshs : 2;
        uint8_t m_pmce_reserved : 2;
        uint8_t m_pmce_compression_level_m2 : 3;
        uint8_t m_pmce_send_no_context_takeover : 1;
        // 1
        uint8_t m_pmce_send_window_bits : 4;
        uint8_t m_pmce_receive_window_bits : 4;
        // 2
        WSF_State m_wsf;
        // 3
        uint8_t m_opcode : 4;
        uint8_t m_rsv3 : 1;
        uint8_t m_rsv2 : 1;
        uint8_t m_rsv1 : 1;
        uint8_t m_fin : 1;
      };
    };

  public:
    // Constructs a parser for incoming frames.
    WebSocket_Frame_Parser();

  public:
    WebSocket_Frame_Parser(const WebSocket_Frame_Parser&) = delete;
    WebSocket_Frame_Parser& operator=(const WebSocket_Frame_Parser&) & = delete;
    ~WebSocket_Frame_Parser();

    // Get configuration values.
    int
    default_compression_level() const noexcept
      { return this->m_default_compression_level;  }

    uint32_t
    max_message_length() const noexcept
      { return this->m_max_message_length;  }

    // Has an error occurred?
    bool
    error() const noexcept
      { return this->m_wsf == wsf_error;  }

    const char*
    error_description() const noexcept
      { return this->m_error_desc ? this->m_error_desc : "success";  }

    // Get the operating mode.
    bool
    is_client_mode() const noexcept
      { return this->m_wshs == wshs_c_accepted;  }

    bool
    is_server_mode() const noexcept
      { return this->m_wshs == wshs_s_accepted;  }

    // Get parameters of the per-message compression extension (PMCE). If PMCE is
    // not active, these functions return zero.
    int
    pmce_compression_level() const noexcept
      { return this->m_pmce_compression_level_m2 + 2;  }

    bool
    pmce_send_no_context_takeover() const noexcept
      { return this->m_pmce_send_no_context_takeover;  }

    uint8_t
    pmce_send_window_bits() const noexcept
      { return this->m_pmce_send_window_bits;  }

    uint8_t
    pmce_receive_window_bits() const noexcept
      { return this->m_pmce_receive_window_bits;  }

    // Clears all fields. This function shall not be called unless the parser is
    // to be reused for another stream.
    void
    clear() noexcept;

    // Deallocate dynamic memory, if any.
    void
    deallocate() noexcept;

    // Creates a WebSocket handshake request from a client. The user may modify
    // the request URI or append new headers after this function returns. The
    // request headers shall be sent to a WebSocket server without a request body.
    // This function requests PMCE. If PMCE is not desired, the caller should
    // remove `Sec-WebSocket-Extensions` from `req.headers` after this function
    // returns.
    void
    create_handshake_request(HTTP_Request_Headers& req);

    // Accepts a WebSocket handshake request from a client, and composes a
    // corresponding response from the server. The response headers shall be sent
    // back to the WebSocket client verbatim, without a response body.
    // This function enables PMCE if the client has requested it. If PMCE is not
    // desired, the caller should remove `Sec-WebSocket-Extensions` from
    // `resp.headers` after this function returns.
    // This function doesn't report errors directly. After this function returns,
    // if `resp.status` equals `101`, a WebSocket connection will have been
    // established. Otherwise, the handshake will have failed and the connection
    // should be closed after the response message.
    void
    accept_handshake_request(HTTP_Response_Headers& resp, const HTTP_Request_Headers& req);

    // Accepts a WebSocket handshake response from the server. Users shall check
    // whether `resp.status` equals `101`.
    void
    accept_handshake_response(const HTTP_Response_Headers& resp);

    // Parses the header of a WebSocket frame. `data` may be consumed partially,
    // and must be preserved between calls. If `frame_header_complete()` returns
    // `true` before the call, this function does nothing.
    void
    parse_frame_header_from_stream(linear_buffer& data);

    // Get the parsed frame header.
    bool
    frame_header_complete() const noexcept
      { return this->m_wsf >= wsf_header_done;  }

    const WebSocket_Frame_Header&
    frame_header() const noexcept
      { return this->m_frm_header;  }

    WebSocket_Frame_Header&
    mut_frame_header() noexcept
      { return this->m_frm_header;  }

    // Parses the payload of a frame. `data` may be consumed partially, and must
    // be preserved between calls. If `frame_payload_complete()` returns `true`
    // before the call, this function does nothing.
    void
    parse_frame_payload_from_stream(linear_buffer& data);

    // Get the parsed frame payload. If the frame has been masked, this buffer
    // contains unmasked data.
    bool
    frame_payload_complete() const noexcept
      { return this->m_wsf >= wsf_payload_done;  }

    const linear_buffer&
    frame_payload() const noexcept
      { return this->m_frm_payload;  }

    linear_buffer&
    mut_frame_payload() noexcept
      { return this->m_frm_payload;  }

    // Get header bits of the current (maybe fragmented) data message, which can
    // be `1` for a text message or `2` for a binary message. If this function is
    // called after a non-FIN data frame or a control frame following a non-FIN
    // data frame, the opcode of the data frame is returned; otherwise, zero is
    // returned.
    bool
    message_fin() const noexcept
      { return this->m_fin;  }

    bool
    message_rsv1() const noexcept
      { return this->m_rsv1;  }

    bool
    message_rsv2() const noexcept
      { return this->m_rsv2;  }

    bool
    message_rsv3() const noexcept
      { return this->m_rsv3;  }

    uint8_t
    message_opcode() const noexcept
      { return this->m_opcode;  }

    // Clears the current complete frame, so the parser can start the next one.
    void
    next_frame() noexcept
      {
        ROCKET_ASSERT(this->m_wsf >= wsf_header_done);

        this->m_frm_header.clear();
        this->m_frm_payload.clear();
        this->m_frm_payload_rem = 0;
        this->m_wsf = wsf_new;
      }
  };

}  // namespace poseidon
#endif
