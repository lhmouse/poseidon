// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_HTTP_SERVER_SESSION_
#define POSEIDON_SOCKET_HTTP_SERVER_SESSION_

#include "../fwd.hpp"
#include "enums.hpp"
#include "tcp_socket.hpp"
#include "../http/http_request_parser.hpp"
#include "../http/http_s_headers.hpp"
namespace poseidon {

class HTTP_Server_Session
  :
    public virtual TCP_Socket
  {
  private:
    HTTP_Request_Parser m_req_parser;
    atomic_relaxed<bool> m_upgrade_ack;

  public:
    // Constructs a socket for incoming connections.
    HTTP_Server_Session();

  protected:
    // This function implements `TCP_Socket`.
    virtual
    void
    do_on_tcp_stream(linear_buffer& data, bool eof) override;

    // Checks whether the protocol has changed.
    bool
    do_has_upgraded() const noexcept
      { return this->m_upgrade_ack.load();  }

    // This callback is invoked by the network thread after all headers of a
    // request have been received, just before the content of it. This callback
    // is primarily used to examine request paths and headers before message
    // bodies. Sending a 101 response from here switches the connection to
    // `do_on_http_upgraded_stream()`.
    // The default implementation invokes `do_on_http_request_error()` if the
    // request URI is absolute (this usually causes shutdown of the connection),
    // and ignores upgrade requests.
    virtual
    void
    do_on_http_request_headers(HTTP_C_Headers& req, bool eot);

    // This callback is invoked by the network thread for each fragment of the
    // request content that has been received. As with `TCP_Connection::
    // do_on_tcp_stream()`, the argument buffer contains all data that have been
    // accumulated so far and callees are supposed to remove bytes that have been
    // processed. This callback will not not invoked for GET, HEAD, DELETE or
    // CONNECT methods.
    // The default implementation leaves all data alone for consumption by
    // `do_on_http_request_finish()`. For security reasons, the length of the
    // content is checked; an error is reported if it exceeds the
    // `network.http.max_request_content_length` limit in 'main.conf'.
    virtual
    void
    do_on_http_request_content_stream(linear_buffer& data);

    // This callback is invoked by the network thread at the end of a request
    // message. Arguments have the same semantics with the other callbacks.
    // `data` for GET, HEAD, DELETE and CONNECT methods will always be empty.
    // `eot` indicates whether the connection should be closed after this
    // message.
    virtual
    void
    do_on_http_request_finish(HTTP_C_Headers&& req, linear_buffer&& data, bool eot)
      = 0;

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
    do_on_http_request_error(HTTP_Method method, HTTP_Status status)
      = 0;

    // This callback is invoked by the network thread on a connection that has
    // switched to another protocol. Arguments have the same semantics with
    // `TCP_Socket::do_on_tcp_stream()`.
    // The default implementation throws an exception.
    virtual
    void
    do_on_http_upgraded_stream(linear_buffer& data, bool eof);

  public:
    HTTP_Server_Session(const HTTP_Server_Session&) = delete;
    HTTP_Server_Session& operator=(const HTTP_Server_Session&) & = delete;
    virtual ~HTTP_Server_Session();

    // Sends a simple response, possibly with the complete content. Callers should
    // not supply `Content-Length` or `Transfer-Encoding` headers, as they will be
    // rewritten. If `resp.status` equals 1xx, 204 or 304, the HTTP specification
    // requires that the response shall have no message content, in which case
    // `data` and `size` are ignored. The function may rewrite `resp.headers` as
    // necessary.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    http_response(HTTP_Method method, HTTP_S_Headers& resp, chars_view data);

    // Send a response with chunked content, which may contain multiple chunks.
    // Callers should not supply `Transfer-Encoding` headers, as they will be
    // rewritten. The HTTP/1.1 specification says that a chunk of length zero
    // terminates the chunked content; therefore, empty chunks are ignored by
    // `http_chunked_response_send()`. These functions do very little error
    // checking. Calling `http_chunked_response_send()` or
    // `http_chunked_response_finish()` when no chunked response is active is
    // likely to corrupt the connection. These functions may rewrite
    // `resp.headers` as necessary.
    // If these function throw an exception, there is no effect.
    // These functions are thread-safe.
    bool
    http_chunked_response_start(HTTP_Method method, HTTP_S_Headers& resp);

    bool
    http_chunked_response_send(chars_view data);

    bool
    http_chunked_response_finish(HTTP_S_Headers& resp);

    // Sends a default response and shuts down the connection. `status` may be
    // a standard status code, or an integer within [200,599]. Any other value is
    // sanitized to 400.
    bool
    http_shut_down(HTTP_Status status = http_status_bad_request) noexcept;

    bool
    http_shut_down(int status) noexcept;
  };

}  // namespace poseidon
#endif
