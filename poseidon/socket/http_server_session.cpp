// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_server_session.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Server_Session::
HTTP_Server_Session(unique_posix_fd&& fd)
  : TCP_Socket(::std::move(fd))  // server constructor
  {
    this->m_req_parser.emplace();
  }

HTTP_Server_Session::
~HTTP_Server_Session()
  {
  }

void
HTTP_Server_Session::
do_on_tcp_stream(linear_buffer& data, bool eof)
  {
    for(;;) {
      // Check whether the connection has switched to another protocol.
      if(this->m_upgrade_ack.load()) {
        this->m_req_parser.reset();
        return this->do_on_http_upgraded_stream(data, eof);
      }

      // If something has gone wrong, ignore further incoming data.
      if(this->m_req_parser->error()) {
        data.clear();
        return;
      }

      if(!this->m_req_parser->headers_complete()) {
        this->m_req_parser->parse_headers_from_stream(data, eof);

        if(this->m_req_parser->error()) {
          data.clear();
          this->do_on_http_request_error(this->m_req_parser->http_status_from_error());
          return;
        }

        if(!this->m_req_parser->headers_complete())
          return;

        // Check request headers.
        auto payload_type = this->do_on_http_request_headers(this->m_req_parser->mut_headers());
        switch(payload_type) {
          case http_payload_normal:
          case http_payload_empty:
            break;

          case http_payload_connect:
            this->m_req_parser.reset();
            this->m_upgrade_ack.store(true);
            return this->do_on_http_upgraded_stream(data, eof);

          default:
            POSEIDON_THROW((
                "Invalid payload type `$3` returned from `do_http_parser_on_headers_complete()`",
                "[HTTP server session `$1` (class `$2`)]"),
                this, typeid(*this), payload_type);
        }
      }

      if(!this->m_req_parser->payload_complete()) {
        this->m_req_parser->parse_payload_from_stream(data, eof);

        if(this->m_req_parser->error()) {
          data.clear();
          this->do_on_http_request_error(this->m_req_parser->http_status_from_error());
          return;
        }

        this->do_on_http_request_payload_stream(this->m_req_parser->mut_payload());

        if(!this->m_req_parser->payload_complete())
          return;

        // Check request headers and the payload.
        this->do_on_http_request_finish(::std::move(this->m_req_parser->mut_headers()),
                ::std::move(this->m_req_parser->mut_payload()),
                this->m_req_parser->should_close_after_payload());
      }

      this->m_req_parser->next_message();
      POSEIDON_LOG_TRACE(("HTTP parser done: data.size `$1`, eof `$2`"), data.size(), eof);
    }
  }

HTTP_Payload_Type
HTTP_Server_Session::
do_on_http_request_headers(HTTP_Request_Headers& req)
  {
    if((req.method == sref("CONNECT")) || !req.uri.starts_with(sref("/"))) {
      // Reject proxy requests.
      this->do_on_http_request_error(HTTP_STATUS_NOT_IMPLEMENTED);
      return http_payload_normal;
    }

    POSEIDON_LOG_INFO((
        "HTTP server received request: $3 $4",
        "[HTTP server session `$1` (class `$2`)]"),
        this, typeid(*this), req.method, req.uri);

    // The default handler doesn't handle Upgrade requests.
    return http_payload_normal;
  }

void
HTTP_Server_Session::
do_on_http_request_payload_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_http_request_finish()`,
    // but perform some security checks, so we won't be affected by compromized
    // 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_request_content_length = 1048576;

    auto value = conf_file.query("network", "http", "max_request_content_length");
    if(value.is_integer())
      max_request_content_length = value.as_integer();
    else if(!value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.http.max_request_content_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          value, conf_file.path());

    if(max_request_content_length < 0)
      POSEIDON_THROW((
          "`network.http.max_request_content_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_request_content_length, conf_file.path());

    if(data.size() > (uint64_t) max_request_content_length)
      POSEIDON_THROW((
          "HTTP request payload too large: `$3` > `$4`",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_request_content_length);
  }

__attribute__((__noreturn__))
void
HTTP_Server_Session::
do_on_http_upgraded_stream(linear_buffer& data, bool eof)
  {
    POSEIDON_THROW((
        "`do_on_http_upgraded_stream()` not implemented: data.size `$3`, eof `$4`",
        "[HTTP server session `$1` (class `$2`)]"),
        this, typeid(*this), data.size(), eof);
  }

bool
HTTP_Server_Session::
do_http_raw_response(const HTTP_Response_Headers& resp, const char* data, size_t size)
  {
    // Compose the message and send it as a whole.
    tinyfmt_ln fmt;
    resp.encode(fmt);
    fmt.putn(data, size);
    bool sent = this->tcp_send(fmt.data(), fmt.size());

    if(resp.status == HTTP_STATUS_SWITCHING_PROTOCOLS) {
      // For server sessions, this indicates that the server has switched to
      // another protocol. The client might have sent more data before this,
      // which would violate RFC 6455 anyway, so we don't care.
      this->m_upgrade_ack.store(true);
    }

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTP_Server_Session::
http_response_headers_only(HTTP_Response_Headers&& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->do_http_raw_response(resp, "", 0);
  }

bool
HTTP_Server_Session::
http_response(HTTP_Response_Headers&& resp, const char* data, size_t size)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < resp.headers.size();  hindex ++)
      if(ascii_ci_equal(resp.headers.at(hindex).first, sref("Content-Length"))
         || ascii_ci_equal(resp.headers.at(hindex).first, sref("Transfer-Encoding")))
        resp.headers.erase(hindex --);

    // Some responses are required to have no payload payload and require no
    // `Content-Length` header.
    if((resp.status <= 199) || is_any_of(resp.status, { HTTP_STATUS_NO_CONTENT, HTTP_STATUS_NOT_MODIFIED }))
      return this->do_http_raw_response(resp, "", 0);

    // Otherwise, a `Content-Length` is required; otherwise the response would
    // be interpreted as terminating by closure ofthe connection.
    resp.headers.emplace_back(sref("Content-Length"), (double)(int64_t) size);

    return this->do_http_raw_response(resp, data, size);
  }

bool
HTTP_Server_Session::
http_chunked_response_start(HTTP_Response_Headers&& resp)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < resp.headers.size();  hindex ++)
      if(ascii_ci_equal(resp.headers.at(hindex).first, sref("Transfer-Encoding")))
        resp.headers.erase(hindex --);

    // Write a chunked header.
    resp.headers.emplace_back(sref("Transfer-Encoding"), sref("chunked"));

    return this->do_http_raw_response(resp, "", 0);
  }

bool
HTTP_Server_Session::
http_chunked_response_send(const char* data, size_t size)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Ignore empty chunks, which would mark the end of the payload.
    if(size == 0)
      return this->socket_state() <= socket_established;

    // Compose a chunk and send it as a whole. The length of this chunk is
    // written as a hexadecimal integer without the `0x` prefix.
    tinyfmt_ln fmt;
    ::rocket::ascii_numput nump;
    nump.put_XU(size);
    fmt.putn(nump.data() + 2, nump.size() - 2);
    fmt << "\r\n";
    fmt.putn(data, size);
    fmt << "\r\n";
    return this->tcp_send(fmt.data(), fmt.size());
  }

bool
HTTP_Server_Session::
http_chunked_respnse_finish()
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP server session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->tcp_send("0\r\n\r\n", 5);
  }

}  // namespace poseidon
