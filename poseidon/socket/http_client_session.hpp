// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_HTTP_CLIENT_SESSION_
#define POSEIDON_SOCKET_HTTP_CLIENT_SESSION_

#include "../fwd.hpp"
#include "tcp_socket.hpp"
#include "../http/http_request_headers.hpp"
#include "../http/http_response_headers.hpp"
#include <http_parser.h>
namespace poseidon {

class HTTP_Client_Session
  : public TCP_Socket
  {
  private:
    friend class Network_Driver;

    ::http_parser m_parser[1];
    HTTP_Response_Headers m_resp;
    linear_buffer m_body;

  public:
    // Constructs a socket for outgoing connections.
    explicit
    HTTP_Client_Session();

  private:
    inline
    void
    do_http_parser_on_message_begin();

    inline
    void
    do_http_parser_on_status(uint32_t status, const char* str, size_t len);

    inline
    void
    do_http_parser_on_header_field(const char* str, size_t len);

    inline
    void
    do_http_parser_on_header_value(const char* str, size_t len);

    inline
    HTTP_Message_Body_Type
    do_http_parser_on_headers_complete();

    inline
    void
    do_http_parser_on_body(const char* str, size_t len);

    inline
    void
    do_http_parser_on_message_complete(bool close_now);

  protected:
    // This function implements `TCP_Socket`.
    virtual
    void
    do_on_tcp_stream(linear_buffer& data, bool eof) override;

    // This callback is invoked by the network thread after all headers of a
    // response have been received, just before the body of it. Returning
    // `http_message_body_normal` indicates that the response has a body whose
    // length is described by the `Content-Length` or `Transfer-Encoding`
    // header. Returning `http_message_body_empty` indicates that the message
    // does not have a body even if it appears so, such as the response to a
    // HEAD request. Returning `http_message_body_upgrade` causes all further
    // incoming data to be delivered via `do_on_http_upgraded_stream()`. This
    // callback is primarily used to examine the status code before processing
    // response data.
    // The default implementation does not check for HEAD or upgrade responses
    // and returns `http_message_body_normal`.
    virtual
    HTTP_Message_Body_Type
    do_on_http_response_headers(HTTP_Response_Headers& resp);

    // This callback is invoked by the network thread for each fragment of the
    // response body that has been received. As with `TCP_Connection::
    // do_on_tcp_stream()`, the argument buffer contains all data that have
    // been accumulated so far and callees are supposed to remove bytes that
    // have been processed.
    // The default implementation leaves all data alone for consumption by
    // `do_on_http_response_finish()`, but it checks the total length of the
    // body so it will not exceed `network.http.max_response_content_length`
    // in 'main.conf'.
    virtual
    void
    do_on_http_response_body_stream(linear_buffer& data);

    // This callback is invoked by the network thread at the end of a response
    // message. Arguments have the same semantics with the other callbacks.
    // `close_now` indicates whether the response contains `close` in its
    // `Connection` header.
    virtual
    void
    do_on_http_response_finish(HTTP_Response_Headers&& resp, linear_buffer&& data, bool close_now) = 0;

    // This callback is invoked by the network thread on a connection that has
    // switched to another protocol. Arguments have the same semantics with
    // `TCP_Socket::do_on_tcp_stream()`.
    // The default implementation throws an exception.
    virtual
    void
    do_on_http_upgraded_stream(linear_buffer& data, bool eof);

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(HTTP_Client_Session);

    // Send a simple request, possibly with a complete body. Callers should
    // not supply `Content-Length` or `Transfer-Encoding` headers, as they
    // will be rewritten.
    // If these function throw an exception, there is no effect.
    // These functions are thread-safe.
    bool
    http_request(HTTP_Request_Headers&& req, const char* data, size_t size);

    bool
    http_request(HTTP_Request_Headers&& req);

    // Send a request with a chunked body, which may contain multiple chunks.
    // Callers should not supply `Transfer-Encoding` headers, as they will be
    // rewritten. The HTTP/1.1 specification says that a chunk of length zero
    // terminates the chunked body; therefore, empty chunks are ignored by
    // `http_chunked_request_send()`. These functions do little error checking.
    // Calling `http_chunked_request_send()` or `http_chunked_request_finish()`
    // when no chunked request is active will corrupt the connection.
    // If these function throw an exception, there is no effect.
    // These functions are thread-safe.
    bool
    http_chunked_request_start(HTTP_Request_Headers&& req);

    bool
    http_chunked_request_send(const char* data, size_t size);

    bool
    http_chunked_request_finish();
  };

}  // namespace poseidon
#endif
