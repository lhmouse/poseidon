// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_COMPOSITOR_
#define POSEIDON_HTTP_WEBSOCKET_COMPOSITOR_

#include "../fwd.hpp"
#include "websocket_frame_header.hpp"
namespace poseidon {

class WebSocket_Compositor
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

    WSC_Mode m_wsc;
    uint8_t m_data_opcode;
    WebSocket_Frame_Header m_header;

  public:
    // Constructs a parser for incoming frames.
    WebSocket_Compositor() noexcept
      {
        this->clear();
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(WebSocket_Compositor);

    // Has an error occurred?
    bool
    error() const noexcept
      { return this->m_wsc == wsc_error;  }

    // Gets the operating mode.
    bool
    is_client_mode() const noexcept
      { return this->m_wsc == wsc_c_accepted;  }

    bool
    is_server_mode() const noexcept
      { return this->m_wsc == wsc_s_accepted;  }

    // Clears all fields. This function shall not be called unless the parser is
    // to be reused for another stream.
    void
    clear() noexcept
      {
        this->m_wsc = wsc_pending;
        this->m_data_opcode = 0;
        this->m_header.clear();
      }

    // Creates a WebSocket handshake request from a client. The user may modify
    // the request URI or append new headers after this function returns. The
    // request headers shall be sent to a WebSocket server without a request body.
    void
    create_handshake_request(HTTP_Request_Headers& req);

    // Accepts a WebSocket handshake request from a client, and composes a
    // corresponding response from the server. If the client has requested
    // per-message deflate extension, this function also sets up compressors and
    // decompressors. The response headers shall be sent back to the WebSocket
    // client verbatim, without a response body.
    // This function doesn't report errors directly. After this function returns,
    // if `resp.status` equals `101`, a WebSocket connection will have been
    // established. Otherwise, the handshake will have failed and the connection
    // should be closed after the response message.
    void
    accept_handshake_request(HTTP_Response_Headers& resp, const HTTP_Request_Headers& req);

    // Accepts a WebSocket handshake response from the server. Users shall check
    // whether `resp.status` equals `101`. If the server has responded with
    // per-message deflate extension, this function also sets up compressors and
    // decompressors.
    void
    accept_handshake_response(const HTTP_Response_Headers& resp);
  };

}  // namespace poseidon
#endif
