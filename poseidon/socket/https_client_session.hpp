// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_HTTPS_CLIENT_SESSION_
#define POSEIDON_SOCKET_HTTPS_CLIENT_SESSION_

#include "../fwd.hpp"
#include "enums.hpp"
#include "ssl_socket.hpp"
#include "../http/http_response_parser.hpp"
#include "../http/http_request_headers.hpp"
namespace poseidon {

class HTTPS_Client_Session
  :
    public virtual SSL_Socket
  {
  private:
    HTTP_Response_Parser m_resp_parser;
    cow_string m_default_host;
    atomic_relaxed<bool> m_upgrade_ack;

  public:
    // Constructs a socket for outgoing connections.
    HTTPS_Client_Session();

  protected:
    // This function implements `SSL_Socket`.
    virtual
    void
    do_on_ssl_stream(linear_buffer& data, bool eof) override;

    // Checks whether the protocol has changed.
    bool
    do_has_upgraded() const noexcept
      { return this->m_upgrade_ack.load();  }

    // This callback is invoked by the network thread after all headers of a
    // response have been received, just before the payload of it. Returning
    // `http_payload_normal` indicates that the response has a payload whose
    // length is described by the `Content-Length` or `Transfer-Encoding`
    // header. Returning `http_payload_empty` indicates that the message
    // does not have a payload even if it appears so, such as the response to a
    // HEAD request. Returning `http_payload_connect` causes all further
    // incoming data to be delivered via `do_on_https_upgraded_stream()`. This
    // callback is primarily used to examine the status code before processing
    // response data.
    // The default implementation does not check for HEAD or upgrade responses
    // and returns `http_payload_normal`.
    virtual
    HTTP_Payload_Type
    do_on_https_response_headers(HTTP_Response_Headers& resp);

    // This callback is invoked by the network thread for each fragment of the
    // response payload that has been received. As with `SSL_Connection::
    // do_on_ssl_stream()`, the argument buffer contains all data that have
    // been accumulated so far and callees are supposed to remove bytes that
    // have been processed.
    // The default implementation leaves all data alone for consumption by
    // `do_on_https_response_finish()`, but it checks the total length of the
    // payload so it will not exceed `network.http.max_response_content_length`
    // in 'main.conf'.
    virtual
    void
    do_on_https_response_payload_stream(linear_buffer& data);

    // This callback is invoked by the network thread at the end of a response
    // message. Arguments have the same semantics with the other callbacks.
    // `close_now` indicates whether the response contains `close` in its
    // `Connection` header.
    virtual
    void
    do_on_https_response_finish(HTTP_Response_Headers&& resp, linear_buffer&& data,
                                bool close_now) = 0;

    // This callback is invoked by the network thread on a connection that has
    // switched to another protocol. Arguments have the same semantics with
    // `SSL_Socket::do_on_ssl_stream()`.
    // The default implementation throws an exception.
    virtual
    void
    do_on_https_upgraded_stream(linear_buffer& data, bool eof);

    // Sends request headers with some additional data. No error checking is
    // performed. This function is provided for convenience only, and maybe
    // isn't very useful unless for some low-level hacks.
    bool
    do_https_raw_request(const HTTP_Request_Headers& req, chars_view data);

  public:
    HTTPS_Client_Session(const HTTPS_Client_Session&) = delete;
    HTTPS_Client_Session& operator=(const HTTPS_Client_Session&) & = delete;
    virtual ~HTTPS_Client_Session();

    // If no `Host:` headers is supplied for a non-proxy request, use this
    // string.
    const cow_string&
    http_default_host() const noexcept
      { return this->m_default_host;  }

    void
    http_set_default_host(const cow_string& host) noexcept;

    // Sends a simple request, possibly with a complete payload. Callers should
    // not supply `Content-Length` or `Transfer-Encoding` headers, as they
    // will be rewritten.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    http_request(HTTP_Request_Headers&& req, chars_view data);

    // Send a request with a chunked payload, which may contain multiple chunks.
    // Callers should not supply `Transfer-Encoding` headers, as they will be
    // rewritten. The HTTP/1.1 specification says that a chunk of length zero
    // terminates the chunked payload; therefore, empty chunks are ignored by
    // `https_chunked_request_send()`. These functions do little error checking.
    // Calling `https_chunked_request_send()` or `https_chunked_request_finish()`
    // when no chunked request is active will corrupt the connection.
    // If these function throw an exception, there is no effect.
    // These functions are thread-safe.
    bool
    http_chunked_request_start(HTTP_Request_Headers&& req);

    bool
    http_chunked_request_send(chars_view data);

    bool
    http_chunked_request_finish();
  };

}  // namespace poseidon
#endif
