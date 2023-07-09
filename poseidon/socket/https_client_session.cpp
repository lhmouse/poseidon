// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "https_client_session.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTPS_Client_Session::
HTTPS_Client_Session(const SSL_CTX_ptr& ssl_ctx)
  : SSL_Socket(ssl_ctx)  // client constructor
  {
    this->do_ssl_alpn_request("http/1.1");
    this->m_resp_parser.emplace();
  }

HTTPS_Client_Session::
~HTTPS_Client_Session()
  {
  }

void
HTTPS_Client_Session::
do_on_ssl_stream(linear_buffer& data, bool eof)
  {
    for(;;) {
      // Check whether the connection has switched to another protocol.
      if(this->m_upgrade_ack.load()) {
        this->m_resp_parser.reset();
        return this->do_on_https_upgraded_stream(data, eof);
      }

      // If something has gone wrong, ignore further incoming data.
      if(this->m_resp_parser->error()) {
        data.clear();
        return;
      }

      if(!this->m_resp_parser->headers_complete()) {
        this->m_resp_parser->parse_headers_from_stream(data, eof);

        if(this->m_resp_parser->error()) {
          data.clear();
          this->quick_close();
          return;
        }

        if(!this->m_resp_parser->headers_complete())
          return;

        // Check response headers.
        auto body_type = this->do_on_https_response_headers(this->m_resp_parser->mut_headers());
        switch(body_type) {
          case http_message_body_normal:
            break;

          case http_message_body_empty:
            this->m_resp_parser->set_no_body();
            break;

          case http_message_body_connect:
            this->m_resp_parser.reset();
            this->m_upgrade_ack.store(true);
            return this->do_on_https_upgraded_stream(data, eof);

          default:
            POSEIDON_THROW((
                "Invalid body type `$3` returned from `do_http_parser_on_headers_complete()`",
                "[HTTPS client session `$1` (class `$2`)]"),
                this, typeid(*this), body_type);
        }
      }

      if(!this->m_resp_parser->body_complete()) {
        this->m_resp_parser->parse_body_from_stream(data, eof);

        if(this->m_resp_parser->error()) {
          data.clear();
          this->quick_close();
          return;
        }

        this->do_on_https_response_body_stream(this->m_resp_parser->mut_body());

        if(!this->m_resp_parser->body_complete())
          return;

        // Check response headers and the body.
        this->do_on_https_response_finish(::std::move(this->m_resp_parser->mut_headers()),
                ::std::move(this->m_resp_parser->mut_body()),
                this->m_resp_parser->should_close_after_body());
      }

      this->m_resp_parser->next_message();
      POSEIDON_LOG_TRACE(("HTTP parser done: data.size `$1`, eof `$2`"), data.size(), eof);
    }
  }

HTTP_Message_Body_Type
HTTPS_Client_Session::
do_on_https_response_headers(HTTP_Response_Headers& resp)
  {
    POSEIDON_LOG_INFO((
        "HTTPS client received response: $3 $4",
        "[HTTPS client session `$1` (class `$2`)]"),
        this, typeid(*this), resp.status, resp.reason);

    // The default handler doesn't handle HEAD, CONNECT or Upgrade responses.
    return http_message_body_normal;
  }

void
HTTPS_Client_Session::
do_on_https_response_body_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_https_response_finish()`,
    // but perform some safety checks, so we won't be affected by compromized
    // 3rd-party servers.
    const auto conf_file = main_config.copy();
    int64_t max_response_content_length = 1048576;

    auto value = conf_file.query("network", "http", "max_response_content_length");
    if(value.is_integer())
      max_response_content_length = value.as_integer();
    else if(!value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.http.max_response_content_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          value, conf_file.path());

    if(max_response_content_length < 0)
      POSEIDON_THROW((
          "`network.http.max_response_content_length` value `$1` out of range",
          "[in configuration file '$2']"),
          max_response_content_length, conf_file.path());

    if(data.size() > (uint64_t) max_response_content_length)
      POSEIDON_THROW((
          "HTTP response body too large: `$3` > `$4`",
          "[HTTPS client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_response_content_length);
  }

__attribute__((__noreturn__))
void
HTTPS_Client_Session::
do_on_https_upgraded_stream(linear_buffer& data, bool eof)
  {
    POSEIDON_THROW((
        "`do_on_https_upgraded_stream()` not implemented: data.size `$3`, eof `$4`",
        "[HTTPS client session `$1` (class `$2`)]"),
        this, typeid(*this), data.size(), eof);
  }

bool
HTTPS_Client_Session::
do_https_raw_request(const HTTP_Request_Headers& req, const char* data, size_t size)
  {
    // Compose the message and send it as a whole.
    tinyfmt_ln fmt;
    req.encode(fmt);
    fmt.putn(data, size);
    bool sent = this->ssl_send(fmt.data(), fmt.size());

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTPS_Client_Session::
https_request(HTTP_Request_Headers&& req, const char* data, size_t size)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < req.headers.size();  hindex ++)
      if(ascii_ci_equal(req.headers.at(hindex).first, sref("Content-Length"))
          || ascii_ci_equal(req.headers.at(hindex).first, sref("Transfer-Encoding")))
        req.headers.erase(hindex --);

    // By default, request messages do not have bodies. Hence the length is
    // only necessary if the body is non-empty.
    if(size != 0)
      req.headers.emplace_back(sref("Content-Length"), (int64_t) size);

    return this->do_https_raw_request(req, data, size);
  }

bool
HTTPS_Client_Session::
https_chunked_request_start(HTTP_Request_Headers&& req)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < req.headers.size();  hindex ++)
      if(ascii_ci_equal(req.headers.at(hindex).first, sref("Transfer-Encoding")))
        req.headers.erase(hindex --);

    // Write a chunked header.
    req.headers.emplace_back(sref("Transfer-Encoding"), sref("chunked"));

    return this->do_https_raw_request(req, "", 0);
  }

bool
HTTPS_Client_Session::
https_chunked_request_send(const char* data, size_t size)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Ignore empty chunks, which would have marked the end of the body.
    if(size == 0)
      return this->socket_state() <= socket_state_established;

    // Compose a chunk and send it as a whole. The length of this chunk is
    // written as a hexadecimal integer without the `0x` prefix.
    tinyfmt_ln fmt;
    ::rocket::ascii_numput nump;
    nump.put_XU(size);
    fmt.putn(nump.data() + 2, nump.size() - 2);
    fmt << "\r\n";
    fmt.putn(data, size);
    fmt << "\r\n";
    return this->ssl_send(fmt.data(), fmt.size());
  }

bool
HTTPS_Client_Session::
https_chunked_request_finish()
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTPS connection switched to another protocol",
          "[HTTPS client session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->ssl_send("0\r\n\r\n", 5);
  }

}  // namespace poseidon
