// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_client_session.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Client_Session::
HTTP_Client_Session()
  : TCP_Socket()  // client constructor
  {
    ::http_parser_init(this->m_parser, HTTP_RESPONSE);
    this->m_parser->allow_chunked_length = true;
    this->m_parser->data = this;
  }

HTTP_Client_Session::
~HTTP_Client_Session()
  {
  }

void
HTTP_Client_Session::
do_on_tcp_stream(linear_buffer& data, bool eof)
  {
    if(HTTP_PARSER_ERRNO(this->m_parser) == HPE_PAUSED) {
      // The protocol has changed, so HTTP parser objects are no longer useful,
      // so free dynamic memory, if any
      if(ROCKET_UNEXPECT(!this->m_upgrade_done)) {
        ::rocket::reconstruct(&(this->m_resp));
        ::rocket::reconstruct(&(this->m_body));
        this->m_upgrade_done = true;
      }

      // Forward data to the new handler.
      this->do_on_http_upgraded_stream(data, eof);
      return;
    }

    // Parse incoming data and remove parsed bytes from the queue. Errors are
    // passed via exceptions.
#define this   static_cast<HTTP_Client_Session*>(ps->data)
    static constexpr ::http_parser_settings settings[1] =
      {{
        // on_message_begin
        +[](::http_parser* ps)
          {
            this->m_resp.clear();
            this->m_body.clear();
            return 0;
          },

        // on_url
        nullptr,

        // on_status
        +[](::http_parser* ps, const char* str, size_t len)
          {
            this->m_resp.status = ps->status_code;
            if(this->m_resp.reason.empty()) {
              // The reason string is very likely a predefined one, so don't bother
              // allocating storage for it.
              auto stat_sh = sref(::http_status_str((::http_status) ps->status_code));
              if(xmemeq(stat_sh.data(), stat_sh.size(), str, len))
                this->m_resp.reason = stat_sh;
              else
                this->m_resp.reason.assign(str, len);
            }
            else {
              this->m_resp.reason.append(str, len);
            }
            return 0;
          },

        // on_header_field
        +[](::http_parser* ps, const char* str, size_t len)
          {
            if(this->m_resp.headers.empty())
              this->m_resp.headers.emplace_back();

            if(this->m_body.size() != 0) {
              // If `m_body` is not empty, a previous header is now complete,
              // so commit it.
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
            return 0;
          },

        // on_header_value
        +[](::http_parser* ps, const char* str, size_t len)
          {
            // Add a magic character to indicate that a part of the value has
            // been received. This makes `m_body` non-empty.
            if(this->m_body.empty())
               this->m_body.putc('\n');

            // Abuse `m_body` to store the header value.
            this->m_body.putn(str, len);
            return 0;
          },

        // on_headers_complete
        +[](::http_parser* ps)
          {
            if(this->m_body.size() != 0) {
              // If `m_body` is not empty, a previous header is now complete,
              // so commit it.
              const char* value_str = this->m_body.data() + 1;
              ROCKET_ASSERT(value_str[-1] == '\n');  // magic character
              size_t value_len = this->m_body.size() - 1;

              auto& value = this->m_resp.headers.mut_back().second;
              if(value.parse(value_str, value_len) != value_len)
                value.set_string(cow_string(value_str, value_len));

              // Finish abuse of `m_body`.
              this->m_body.clear();
            }

            // The headers are complete, so determine whether a request body
            // should be expected.
            auto body_type = this->do_on_http_response_headers(this->m_resp);
            switch(body_type) {
              case http_message_body_normal:
                return 0;

              case http_message_body_empty:
                return 1;

              case http_message_body_connect:
                return 2;

              default:
                POSEIDON_THROW((
                    "Invalid body type `$3` returned from `do_http_parser_on_headers_complete()`",
                    "[HTTP client session `$1` (class `$2`)]"),
                    this, typeid(*this), body_type);
            }
          },

        // on_body
        +[](::http_parser* ps, const char* str, size_t len)
          {
            this->m_body.putn(str, len);
            this->do_on_http_response_body_stream(this->m_body);
            return 0;
          },

        // on_message_complete
        +[](::http_parser* ps)
          {
            bool close_now = ::http_should_keep_alive(ps);
            this->do_on_http_response_finish(::std::move(this->m_resp), ::std::move(this->m_body), close_now);

            if((ps->http_errno == HPE_OK) && (ps->status_code == HTTP_STATUS_SWITCHING_PROTOCOLS)) {
              // For client sessions, this indicates that the server has
              // switched to another protocol.
              ps->http_errno = HPE_PAUSED;
              this->m_upgrade_ack.store(true);
            }
            return 0;
          },

        // on_chunk_header
        nullptr,

        // on_chunk_complete
        nullptr,
      }};
#undef this

    if(data.size() != 0)
      data.discard(::http_parser_execute(this->m_parser, settings, data.data(), data.size()));

    if(eof)
      ::http_parser_execute(this->m_parser, settings, "", 0);

    // Assuming the error code won't be clobbered by the second call for EOF
    // above. Not sure.
    switch((uint32_t) HTTP_PARSER_ERRNO(this->m_parser)) {
      case HPE_OK:
        break;

      case HPE_PAUSED:
        this->do_on_http_upgraded_stream(data, eof);
        break;

      default:
        POSEIDON_LOG_WARN((
            "HTTP parser error `$3`: $4",
            "[HTTP client session `$1` (class `$2`)]"),
            this, typeid(*this), HTTP_PARSER_ERRNO(this->m_parser),
            ::http_errno_description(HTTP_PARSER_ERRNO(this->m_parser)));

        // This can't be recovered. All further data will be discarded. There
        // is not much we can do.
        this->quick_close();
        break;
    }

    POSEIDON_LOG_TRACE(("HTTP parser done: data.size `$1`, eof `$2`"), data.size(), eof);
  }

HTTP_Message_Body_Type
HTTP_Client_Session::
do_on_http_response_headers(HTTP_Response_Headers& resp)
  {
    POSEIDON_LOG_DEBUG((
        "HTTP client received response: $3 $4",
        "[HTTP client session `$1` (class `$2`)]"),
        this, typeid(*this), resp.status, resp.reason);

    // The default handler doesn't handle HEAD, CONNECT or Upgrade responses.
    return http_message_body_normal;
  }

void
HTTP_Client_Session::
do_on_http_response_body_stream(linear_buffer& data)
  {
    // Leave `data` alone for consumption by `do_on_http_response_finish()`,
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
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this), data.size(), max_response_content_length);
  }

__attribute__((__noreturn__))
void
HTTP_Client_Session::
do_on_http_upgraded_stream(linear_buffer& data, bool eof)
  {
    POSEIDON_THROW((
        "`do_on_http_upgraded_stream()` not implemented: data.size `$3`, eof `$4`",
        "[HTTP client session `$1` (class `$2`)]"),
        this, typeid(*this), data.size(), eof);
  }

bool
HTTP_Client_Session::
do_http_raw_request(const HTTP_Request_Headers& req, const char* data, size_t size)
  {
    // Compose the message and send it as a whole.
    tinyfmt_ln fmt;
    req.encode(fmt);
    fmt.putn(data, size);
    bool sent = this->tcp_send(fmt.data(), fmt.size());

    // The return value indicates whether no error has occurred. There is no
    // guarantee that data will eventually arrive, due to network flapping.
    return sent;
  }

bool
HTTP_Client_Session::
http_request(HTTP_Request_Headers&& req, const char* data, size_t size)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
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

    return this->do_http_raw_request(req, data, size);
  }

bool
HTTP_Client_Session::
http_chunked_request_start(HTTP_Request_Headers&& req)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    // Erase bad headers.
    for(size_t hindex = 0;  hindex < req.headers.size();  hindex ++)
      if(ascii_ci_equal(req.headers.at(hindex).first, sref("Transfer-Encoding")))
        req.headers.erase(hindex --);

    // Write a chunked header.
    req.headers.emplace_back(sref("Transfer-Encoding"), sref("chunked"));

    return this->do_http_raw_request(req, "", 0);
  }

bool
HTTP_Client_Session::
http_chunked_request_send(const char* data, size_t size)
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
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
    return this->tcp_send(fmt.data(), fmt.size());
  }

bool
HTTP_Client_Session::
http_chunked_request_finish()
  {
    if(this->m_upgrade_ack.load())
      POSEIDON_THROW((
          "HTTP connection switched to another protocol",
          "[HTTP client session `$1` (class `$2`)]"),
          this, typeid(*this));

    return this->tcp_send("0\r\n\r\n", 5);
  }

}  // namespace poseidon
