// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_PARSER_
#define POSEIDON_HTTP_WEBSOCKET_PARSER_

#include "../fwd.hpp"
#include "websocket_frame_header.hpp"
namespace poseidon {

class WebSocket_Parser
  {
  private:
    enum WSC_Mode : uint8_t
      {
        wsc_pending     = 0,
        wsc_error       = 1,
        wsc_c_req_sent  = 2,
        wsc_c_accepted  = 3,
        wsc_s_accepted  = 4,
      };

    WebSocket_Frame_Header m_frm_header;
    linear_buffer m_frm_payload;
    linear_buffer m_msg;
    const char* m_error_description;

    WSC_Mode m_wsc;
    char m_reserved_1;
    uint8_t m_pmce_max_window_bits;
    bool m_pmce_no_context_takeover;

    bool m_frm_header_complete;
    bool m_frm_payload_complete;
    uint8_t m_msg_fin : 1;
    uint8_t m_msg_rsv1 : 1;
    uint8_t m_msg_rsv2 : 1;
    uint8_t m_msg_rsv3 : 1;
    uint8_t m_msg_opcode : 4;
    char m_reserved_2;

  public:
    // Constructs a parser for incoming frames.
    WebSocket_Parser() noexcept
      {
        this->clear();
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(WebSocket_Parser);

    // Has an error occurred?
    bool
    error() const noexcept
      { return this->m_wsc == wsc_error;  }

    const char*
    error_description() const noexcept
      { return this->m_error_description;  }

    // Get the operating mode.
    bool
    is_client_mode() const noexcept
      { return this->m_wsc == wsc_c_accepted;  }

    bool
    is_server_mode() const noexcept
      { return this->m_wsc == wsc_s_accepted;  }

    // Get parameters of the per-message compression extension (PMCE). If PMCE is
    // not active, these functions return zero.
    int
    pmce_max_window_bits() const noexcept
      { return this->m_pmce_max_window_bits;  }

    bool
    pmce_no_context_takeover() const noexcept
      { return this->m_pmce_no_context_takeover;  }

    // Clears all fields. This function shall not be called unless the parser is
    // to be reused for another stream.
    void
    clear() noexcept
      {
        this->m_frm_header.clear();
        this->m_frm_payload.clear();
        this->m_msg.clear();
        this->m_error_description = "";

        this->m_wsc = wsc_pending;
        this->m_reserved_1 = 0;
        this->m_pmce_max_window_bits = 0;
        this->m_pmce_no_context_takeover = false;

        this->m_frm_header_complete = false;
        this->m_frm_payload_complete = false;
        this->m_msg_fin = 0;
        this->m_msg_rsv1 = 0;
        this->m_msg_rsv2 = 0;
        this->m_msg_rsv3 = 0;
        this->m_msg_opcode = 0;
        this->m_reserved_2 = 0;
      }

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
    parse_frame_header_from_stream(linear_buffer& data, bool eof);

    // Get the parsed frame header.
    bool
    frame_header_complete() const noexcept
      { return this->m_frm_header_complete;  }

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
    parse_frame_payload_from_stream(linear_buffer& data, bool eof);

    // Get the parsed frame payload. If the frame is a masked frame, this buffer
    // contains unmasked data. Users should only call this function to fetch the
    // payload of a control frame, as they can't be fragmented. See also the
    // `combined_message()` function.
    bool
    frame_payload_complete() const noexcept
      { return this->m_frm_payload_complete;  }

    const linear_buffer&
    frame_payload() const noexcept
      { return this->m_frm_payload;  }

    linear_buffer&
    mut_frame_payload() noexcept
      { return this->m_frm_payload;  }

    // Get the combined (maybe partial) data message. Control messages will not
    // be combined into this buffer. See also the `frame_payload()` function.
    bool
    message_fin() const noexcept
      { return this->m_msg_fin;  }

    bool
    message_rsv1() const noexcept
      { return this->m_msg_rsv1;  }

    bool
    message_rsv2() const noexcept
      { return this->m_msg_rsv2;  }

    bool
    message_rsv3() const noexcept
      { return this->m_msg_rsv3;  }

    uint8_t
    message_opcode() const noexcept
      { return this->m_msg_opcode;  }

    const linear_buffer&
    combined_message() const noexcept
      { return this->m_msg;  }

    linear_buffer&
    mut_combined_message() noexcept
      { return this->m_msg;  }

    // Clears the current complete frame, so the parser can start the next one.
    void
    next_frame() noexcept
      {
        ROCKET_ASSERT(this->m_frm_payload_complete);
        this->m_frm_header.clear();
        this->m_frm_payload.clear();
        this->m_frm_header_complete = false;
        this->m_frm_payload_complete = false;
      }
  };

}  // namespace poseidon
#endif
