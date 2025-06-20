// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_WS_SERVER_SESSION_
#define POSEIDON_SOCKET_WS_SERVER_SESSION_

#include "../fwd.hpp"
#include "enums.hpp"
#include "http_server_session.hpp"
#include "../http/websocket_frame_parser.hpp"
namespace poseidon {

class WS_Server_Session
  :
    public virtual HTTP_Server_Session
  {
  private:
    WebSocket_Frame_Parser m_parser;
    shptr<WebSocket_Deflator> m_pmce_opt;
    linear_buffer m_msg;
    bool m_closure_notified = false;

  public:
    // Constructs a socket for incoming connections.
    WS_Server_Session();

  private:
    void
    do_call_on_ws_close_once(WebSocket_Status status, chars_view reason);

  protected:
    // This function implements `HTTP_Server_Session`.
    virtual
    void
    do_abstract_socket_on_closed() override;

    virtual
    HTTP_Payload_Type
    do_on_http_request_headers(HTTP_C_Headers& req, bool close_after_payload) override;

    virtual
    void
    do_on_http_request_payload_stream(linear_buffer& data) override;

    virtual
    void
    do_on_http_request_finish(HTTP_C_Headers&& req, linear_buffer&& data, bool eot) override;

    virtual
    void
    do_on_http_request_error(HTTP_Status status) override;

    virtual
    void
    do_on_http_upgraded_stream(linear_buffer& data, bool eof) override;

    // This callback is invoked by the network thread when a WebSocket connection
    // has been accepted. The argument is the request URI of the client.
    // The default implementation does nothing.
    virtual
    void
    do_on_ws_accepted(cow_string&& caddr);

    // This callback is invoked by the network thread for each fragment of a data
    // message. `opcode` indicates the type of the message, which can be either
    // `websocket_text` or `websocket_binary`; control frames are never notified via
    // this callback. As with `TCP_Connection::do_on_tcp_stream()`, the argument
    // buffer contains all data that have been accumulated so far and callees are
    // supposed to remove bytes that have been processed.
    // The default implementations leave all data alone for consumption by
    // `do_on_ws_text_data()`. For security reasons, the total length of the
    // message payload is checked; an error is reported if it exceeds the
    // `network.http.max_websocket_message_length` limit in 'main.conf'.
    virtual
    void
    do_on_ws_message_data_stream(WebSocket_Opcode opcode, linear_buffer& data);

    // This callback is invoked by the network thread at the end of a data message
    // or a control frame. `opcode` may be `websocket_text`, `websocket_binary`,
    // `websocket_ping` or `websocket_pong`.
    virtual
    void
    do_on_ws_message_finish(WebSocket_Opcode opcode, linear_buffer&& data) = 0;

    // This callback is invoked by the network thread when an error occurs, or
    // after a CLOSE frame has been received. The connection will be closed after
    // this function returns.
    // The default implementation does nothing.
    virtual
    void
    do_on_ws_close(WebSocket_Status status, chars_view reason);

    // This function shall be called by `do_on_http_request_headers()` to
    // establish a WebSocket connection.
    void
    do_ws_complete_handshake(HTTP_C_Headers& req, bool close_after_payload);

    // Sends a raw frame (not a message). No error checking is performed. This
    // function is provided for convenience only, and maybe isn't very useful
    // unless for some low-level hacks.
    bool
    do_ws_send_raw_frame(int rsv_opcode, chars_view data);

  public:
    WS_Server_Session(const WS_Server_Session&) = delete;
    WS_Server_Session& operator=(const WS_Server_Session&) & = delete;
    virtual ~WS_Server_Session();

    // Sends a data message or control frame to the other peer. `opcode` indicates
    // the type of the message.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    ws_send(WebSocket_Opcode opcode, chars_view data);

    // Sends a CLOSE frame with an optional error message, then shuts down the
    // connection. The reason string will be truncated to 123 bytes if it's too
    // long.
    // This function is thread-safe.
    bool
    ws_shut_down(WebSocket_Status status = websocket_status_normal_closure,
                 chars_view reason = "") noexcept;
  };

}  // namespace poseidon
#endif
