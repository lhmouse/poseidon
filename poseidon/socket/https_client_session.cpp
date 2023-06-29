// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "https_client_session.hpp"
#include "base/config_file.hpp"
#include "static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTPS_Client_Session::
HTTPS_Client_Session(const SSL_CTX_ptr& ssl_ctx)
  : SSL_Socket(ssl_ctx)  // client constructor
  {
    this->do_ssl_alpn_request("http/1.1");
    ::http_parser_init(this->m_parser, HTTP_RESPONSE);
    this->m_parser->data = this;
  }

HTTPS_Client_Session::
~HTTPS_Client_Session()
  {
  }

void
HTTPS_Client_Session::
do_http_parser_on_message_begin()
  {
    this->m_resp.status = 0;
    this->m_resp.reason.clear();
    this->m_resp.headers.clear();
    this->m_body.clear();
  }

void
HTTPS_Client_Session::
do_http_parser_on_status(uint32_t status, const char* str, size_t len)
  {
    this->m_resp.status = status;

    if(this->m_resp.reason.empty()) {
      // The reason string is very likely a predefined one, so don't bother
      // allocating storage for it.
      auto static_reason = sref(::http_status_str((::http_status) this->m_resp.status));
      if((static_reason.length() == len) && (::memcmp(static_reason.c_str(), str, len) == 0))
        this->m_resp.reason = static_reason;
      else
        this->m_resp.reason.assign(str, len);
    }
    else
      this->m_resp.reason.append(str, len);
  }

void
HTTPS_Client_Session::
do_http_parser_on_header_field(const char* str, size_t len)
  {
    if(this->m_resp.headers.empty())
      this->m_resp.headers.emplace_back();

    if(this->m_body.size() != 0) {
      // If `m_body` is not empty, a previous header will now be complete, so
      // commit it.
      const char* value_str = this->m_body.data() + 1;
      ROCKET_ASSERT(value_str[-1] == '\n');  // magic character
      size_t value_len = this->m_body.size() - 1;

      auto& value = this->m_resp.headers.mut_back().second;
      if(value.parse(value_str, value_len) != value_len)
        value.set_string(cow_string(value_str, value_len));

      // Create the next header.
      this->m_resp.headers.emplace_back();
      this->m_body.clear();
    }

    // Append the header name to the last key, as this callback might
    // be invoked repeatedly.
    this->m_resp.headers.mut_back().first.append(str, len);
  }

void
HTTPS_Client_Session::
do_http_parser_on_header_value(const char* str, size_t len)
  {
    // Add a magic character to indicate that a part of the value has been
    // received. This makes `m_body` non-empty.
    if(this->m_body.empty())
       this->m_body.putc('\n');

    // Abuse `m_body` to store the header value.
    this->m_body.putn(str, len);
  }

HTTP_Message_Body_Type
HTTPS_Client_Session::
do_http_parser_on_headers_complete()
  {
    if(this->m_body.size() != 0) {
      // If `m_body` is not empty, a previous header will now be complete, so
      // commit it.
      const char* value_str = this->m_body.data() + 1;
      ROCKET_ASSERT(value_str[-1] == '\n');  // magic character
      size_t value_len = this->m_body.size() - 1;

      auto& value = this->m_resp.headers.mut_back().second;
      if(value.parse(value_str, value_len) != value_len)
        value.set_string(cow_string(value_str, value_len));

      // Finish abuse of `m_body`.
      this->m_body.clear();
    }

    // The headers are complete, so determine whether we want a response body.
    return this->do_on_https_response_headers(this->m_resp);
  }

void
HTTPS_Client_Session::
do_http_parser_on_body(const char* str, size_t len)
  {
    this->m_body.putn(str, len);
    this->do_on_https_response_body_stream(this->m_body);
  }

void
HTTPS_Client_Session::
do_http_parser_on_message_complete(bool close_now)
  {
    this->do_on_https_response_finish(::std::move(this->m_resp), ::std::move(this->m_body), close_now);
  }

void
HTTPS_Client_Session::
do_on_ssl_stream(linear_buffer& data, bool eof)
  {
    if(this->m_upgrade_ack.load()) {
      this->do_on_https_upgraded_stream(data, eof);
      return;
    }

    // Parse incoming data and remove parsed bytes from the queue. Errors are
    // passed via exceptions.
    static constexpr ::http_parser_settings settings[1] =
      {{
        // on_message_begin
        +[](::http_parser* ps)
          {
            const auto xthis = (HTTPS_Client_Session*) ps->data;
            xthis->do_http_parser_on_message_begin();
            return 0;
          },

        // on_url
        nullptr,

        // on_status
        +[](::http_parser* ps, const char* str, size_t len)
          {
            const auto xthis = (HTTPS_Client_Session*) ps->data;
            xthis->do_http_parser_on_status(ps->status_code, str, len);
            return 0;
          },

        // on_header_field
        +[](::http_parser* ps, const char* str, size_t len)
          {
            const auto xthis = (HTTPS_Client_Session*) ps->data;
            xthis->do_http_parser_on_header_field(str, len);
            return 0;
          },

        // on_header_value
        +[](::http_parser* ps, const char* str, size_t len)
          {
            const auto xthis = (HTTPS_Client_Session*) ps->data;
            xthis->do_http_parser_on_header_value(str, len);
            return 0;
          },

        // on_headers_complete
        +[](::http_parser* ps)
          {
            const auto xthis = (HTTPS_Client_Session*) ps->data;
            auto body_type = xthis->do_http_parser_on_headers_complete();
            switch(body_type) {
              case http_message_body_normal:
                return 0;

              case http_message_body_empty:
                return 1;

              case http_message_body_upgrade:
                return 2;

              default:
                POSEIDON_THROW((
                    "Invalid body type `$3` returned from `do_http_parser_on_headers_complete()`",
                    "[HTTPS client session `$1` (class `$2`)]"),
                    xthis, typeid(*xthis), body_type);
            }
          },

        // on_body
        +[](::http_parser* ps, const char* str, size_t len)
          {
            const auto xthis = (HTTPS_Client_Session*) ps->data;
            xthis->do_http_parser_on_body(str, len);
            return 0;
          },

        // on_message_complete
        +[](::http_parser* ps)
          {
            const auto xthis = (HTTPS_Client_Session*) ps->data;
            xthis->do_http_parser_on_message_complete(::http_should_keep_alive(ps) == 0);

            if((ps->http_errno == HPE_OK) && (ps->status_code == HTTP_STATUS_SWITCHING_PROTOCOLS)) {
              // For client sessions, this indicates that the server has
              // switched to another protocol.
              ps->http_errno = HPE_PAUSED;
              xthis->m_upgrade_ack.store(true);
            }
            return 0;
          },

        // on_chunk_header
        nullptr,

        // on_chunk_complete
        nullptr,
      }};

    if(data.size() != 0)
      data.discard(::http_parser_execute(this->m_parser, settings, data.data(), data.size()));

    if(eof)
      ::http_parser_execute(this->m_parser, settings, "", 0);

    // Assuming the error code won't be clobbered by the second call for EOF
    // above. Not sure.
    if(HTTP_PARSER_ERRNO(this->m_parser) != HPE_OK) {
      // This can't be recovered. All further data will be discarded. There is
      // not much we can do.
      POSEIDON_LOG_WARN((
          "HTTP parser error `$3`: $4",
          "[HTTPS client session `$1` (class `$2`)]"),
          this, typeid(*this), HTTP_PARSER_ERRNO(this->m_parser),
          ::http_errno_description(HTTP_PARSER_ERRNO(this->m_parser)));

      data.clear();
      return;
    }

    POSEIDON_LOG_TRACE(("HTTPS parser done: data.size `$1`, eof `$2`"), data.size(), eof);
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
    tinyfmt_str fmt;
    fmt << req;
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
      if(req.header_name_equals(hindex, sref("Content-Length"))
          || req.header_name_equals(hindex, sref("Transfer-Encoding")))
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
      if(req.header_name_equals(hindex, sref("Transfer-Encoding")))
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
    tinyfmt_str fmt;
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
