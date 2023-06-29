// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_HTTPS_SERVER_SESSION_
#define POSEIDON_SOCKET_HTTPS_SERVER_SESSION_

#include "../fwd.hpp"
#include "ssl_socket.hpp"
#include "../http/http_request_headers.hpp"
#include "../http/http_response_headers.hpp"
#include <http_parser.h>
namespace poseidon {

class HTTPS_Server_Session
  : public SSL_Socket
  {
  private:
    friend class Network_Driver;

    ::http_parser m_parser[1];
    HTTP_Request_Headers m_req;
    linear_buffer m_body;
    atomic_relaxed<bool> m_upgrade_ack;

  public:
    // Constructs a socket for incoming connections.
    explicit
    HTTPS_Server_Session(unique_posix_fd&& fd, const SSL_CTX_ptr& ssl_ctx);

  private:
    inline
    void
    do_http_parser_on_message_begin();

    inline
    void
    do_http_parser_on_uri(uint32_t method, const char* str, size_t len);

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
    // These function implement `SSL_Socket`.
    virtual
    void
    do_on_ssl_stream(linear_buffer& data, bool eof) override;

    virtual
    char256
    do_on_ssl_alpn_request(cow_vector<char256>&& protos) override;

    // This callback is invoked by the network thread after all headers of a
    // request have been received, just before the body of it. Returning
    // `http_message_body_normal` indicatesthat the request has a body whose
    // length is described by the `Content-Length` or `Transfer-Encoding`
    // header. Returning `http_message_body_empty` indicates that the message
    // does not have a body even if it appears so, such as a CONNECT request.
    // Returning `http_message_body_upgrade` causes all further incoming data
    // to be delivered via `do_on_http_upgraded_stream()`. This callback is
    // primarily used to examine request paths and headers before processing
    // request data.
    // The default implementation invokes `do_on_http_request_error()` if the
    // request method is CONNECT or the request URI is absolute (this usually
    // causes shutdown of the connection), ignores upgrade requests, and
    // returns `http_message_body_normal`.
    virtual
    HTTP_Message_Body_Type
    do_on_https_request_headers(HTTP_Request_Headers& req);

    // This callback is invoked by the network thread for each fragment of the
    // request body that has been received. As with `SSL_Connection::
    // do_on_ssl_stream()`, the argument buffer contains all data that have
    // been accumulated so far and callees are supposed to remove bytes that
    // have been processed. This callback will not not invoked for GET, HEAD,
    // DELETE or CONNECT methods.
    // The default implementation leaves all data alone for consumption by
    // `do_on_https_request_finish()`, but it checks the total length of the
    // body so it will not exceed `network.http.max_request_content_length`
    // in 'main.conf'.
    virtual
    void
    do_on_https_request_body_stream(linear_buffer& data);

    // This callback is invoked by the network thread at the end of a request
    // message. Arguments have the same semantics with the other callbacks.
    // `data` for GET, HEAD, DELETE and CONNECT methods will always be empty.
    // `close_now` indicates whether the request contains `close` in its
    // `Connection` header.
    virtual
    void
    do_on_https_request_finish(HTTP_Request_Headers&& req, linear_buffer&& data, bool close_now) = 0;

    // This callback is invoked when an HTTP parser error happens. Why must we
    // dedicate an error callback for server sessions? Well, it's because HTTP
    // pipelining: HTTP responses must be sent in the exact order as their
    // corresponding requests. Our recommended setup is to have the network
    // thread parse HTTP requests and to have the main thread process them in
    // fibers. Hence, when the network thread effects an exception, there may
    // still be pending requests in the main thread. We cannot send an error
    // response right now; instead, we have to enqueue this error to the main
    // thread, after all requests, to preserve the order of responses.
    virtual
    void
    do_on_https_request_error(uint32_t status) = 0;

    // This callback is invoked by the network thread on a connection that has
    // switched to another protocol. Arguments have the same semantics with
    // `SSL_Socket::do_on_ssl_stream()`.
    // The default implementation throws an exception.
    virtual
    void
    do_on_https_upgraded_stream(linear_buffer& data, bool eof);

    // Sends response headers with some additional data. No error checking is
    // performed. This function is provided for convenience only, and maybe
    // isn't very useful unless for some low-level hacks.
    bool
    do_https_raw_response(const HTTP_Response_Headers& resp, const char* data, size_t size);

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(HTTPS_Server_Session);

    // Sends a headers-only response. This can be used to respond to a HEAD or
    // CONNECT request, or to indicate that the message body terminates until
    // closure of the connection. In the latter case, callers must not supply
    // `Content-Length` or `Transfer-Encoding` headers, and shall send further
    // data with `ssl_send()`.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    https_response_headers_only(HTTP_Response_Headers&& resp);

    // Sends a simple response, possibly with a complete body. Callers should
    // not supply `Content-Length` or `Transfer-Encoding` headers, as they
    // will be rewritten. If `resp.status` equals 1xx, 204 or 304, the HTTP
    // specification requires that the response shall have no message body, in
    // which case `data` and `size` are ignored.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    https_response(HTTP_Response_Headers&& resp, const char* data, size_t size);

    // Send a response with a chunked body, which may contain multiple chunks.
    // Callers should not supply `Transfer-Encoding` headers, as they will be
    // rewritten. The HTTP/1.1 specification says that a chunk of length zero
    // terminates the chunked body; therefore, empty chunks are ignored by
    // `http_chunked_response_send()`. These functions do very little error
    // checking. Calling `http_chunked_response_send()` or
    // `http_chunked_response_finish()` when no chunked response is active is
    // likely to corrupt the connection.
    // If these function throw an exception, there is no effect.
    // These functions are thread-safe.
    bool
    https_chunked_response_start(HTTP_Response_Headers&& resp);

    bool
    https_chunked_response_send(const char* data, size_t size);

    bool
    https_chunked_respnse_finish();
  };

}  // namespace poseidon
#endif
