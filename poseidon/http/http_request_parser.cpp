// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_request_parser.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

POSEIDON_VISIBILITY_HIDDEN
const ::http_parser_settings
HTTP_Request_Parser::s_settings[1] =
#define this   static_cast<HTTP_Request_Parser*>(ps->data)
  {{
    // on_message_begin
    nullptr,

    // on_url
    +[](::http_parser* ps, const char* str, size_t len)
      {
        this->m_headers.raw_host.append(str, len);
        return 0;
      },

    // on_status
    nullptr,

    // on_header_field
    +[](::http_parser* ps, const char* str, size_t len)
      {
        // If this notification is received when no header exists, or a previous
        // header value has been accepted, then a new header starts.
        if(this->m_headers.headers.empty() || !this->m_headers.headers.back().second.is_null())
          this->m_headers.headers.emplace_back();

        // Append the header name to the last key, as this callback might be
        // invoked repeatedly.
        this->m_headers.headers.mut_back().first.mut_str().append(str, len);
        return 0;
      },

    // on_header_value
    +[](::http_parser* ps, const char* str, size_t len)
      {
        // Append the header value, as this callback might be invoked repeatedly.
        cow_string value = this->m_headers.headers.back().second.as_string();
        value.append(str, len);
        this->m_headers.headers.mut_back().second = move(value);
        return 0;
      },

    // on_headers_complete
    +[](::http_parser* ps)
      {
        // Set the method string. This might not be null-terminated.
        const char* method_str = ::http_method_str(static_cast<::http_method>(ps->method));
        if(!::memccpy(this->m_headers.method_str, method_str, 0, sizeof(this->m_headers.method_str))) {
          ps->http_errno = HPE_INVALID_METHOD;
          return 0;
        }

        // Parse the URI.
        cow_string caddr;
        caddr.swap(this->m_headers.raw_host);

        ::http_parser_url uri_hp;
        ::http_parser_url_init(&uri_hp);
        if(::http_parser_parse_url(caddr.data(), caddr.size(), ps->method == HTTP_CONNECT, &uri_hp)) {
          ps->http_errno = HPE_INVALID_URL;
          return 0;
        }

#define uri_has_(x)  ((uri_hp.field_set & (1U << UF_##x)) != 0)
#define uri_str_(x)  (caddr.data() + uri_hp.field_data[UF_##x].off)
#define uri_len_(x)  (uri_hp.field_data[UF_##x].len)

        if(uri_has_(SCHEMA)) {
          // The scheme shall be `http://` or `https://`. The default port is set
          // accordingly. When no scheme is specified, the default port is left
          // zero.
          if(::rocket::ascii_ci_equal(uri_str_(SCHEMA), uri_len_(SCHEMA), "http", 4)) {
            this->m_headers.port = 80;
            this->m_headers.is_proxy = true;
            this->m_headers.is_ssl = false;
          }
          else if(::rocket::ascii_ci_equal(uri_str_(SCHEMA), uri_len_(SCHEMA), "https", 5)) {
            this->m_headers.port = 443;
            this->m_headers.is_proxy = true;
            this->m_headers.is_ssl = true;
          }
          else {
            ps->http_errno = HPE_INVALID_URL;
            return 2;
          }
        }

        if(uri_has_(HOST)) {
          // Use the host name in the request URI and ignore the `Host:` header.
          this->m_headers.is_proxy = true;
          this->m_headers.raw_host.assign(uri_str_(HOST), uri_len_(HOST));
        }
        else {
          // Get the `Host:` header. Multiple `Host:` headers are not allowed.
          uint32_t host_header_num = 0;
          for(const auto& hr : this->m_headers.headers)
            if(hr.first == "Host") {
              host_header_num ++;
              if(host_header_num == 1)
                this->m_headers.raw_host = hr.second.as_string();
              else
                break;
            }

          if(host_header_num != 1) {
            ps->http_errno = HPE_INVALID_URL;
            return 2;
          }
        }

        if(uri_has_(PORT))
          this->m_headers.port = uri_hp.port;

        this->m_headers.raw_userinfo.assign(uri_str_(USERINFO), uri_len_(USERINFO));
        this->m_headers.raw_path.assign(uri_str_(PATH), uri_len_(PATH));
        this->m_headers.raw_query.assign(uri_str_(QUERY), uri_len_(QUERY));

        // The headers are complete, so halt.
        this->m_hreq = hreq_headers_done;
        this->m_close_after_payload = ::http_should_keep_alive(ps) == 0;
        ps->http_errno = HPE_PAUSED;
        return 0;
      },

    // on_body
    +[](::http_parser* ps, const char* str, size_t len)
      {
        switch(ps->method)
          {
          case HTTP_POST:
          case HTTP_PUT:
          case HTTP_PATCH:
            // Accept the payload.
            this->m_payload.putn(str, len);
            return 0;

          default:
            // Discard the payload.
            return 0;
          }
      },

    // on_message_complete
    +[](::http_parser* ps)
      {
        // The message is complete, so halt.
        this->m_hreq = hreq_payload_done;
        ps->http_errno = HPE_PAUSED;
        return 0;
      },

    // on_chunk_header
    nullptr,

    // on_chunk_complete
    nullptr,
  }};
#undef this

HTTP_Request_Parser::
HTTP_Request_Parser()
  {
    ::http_parser_init(this->m_parser, HTTP_REQUEST);
    this->m_parser->data = this;

    const auto conf_file = main_config.copy();
    auto conf_value = conf_file.query(&"network.http.default_compression_level");
    if(conf_value.is_integer())
      this->m_default_compression_level = clamp_cast<int>(conf_value.as_integer(), 0, 9);
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.http.default_compression_level`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    conf_value = conf_file.query(&"network.http.max_request_content_length");
    if(conf_value.is_integer())
      this->m_max_content_length = clamp_cast<uint32_t>(conf_value.as_integer(), 0x100, 0x10000000);
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.http.max_request_content_length`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());
  }

HTTP_Request_Parser::
~HTTP_Request_Parser()
  {
  }

HTTP_Status
HTTP_Request_Parser::
http_status_from_error() const noexcept
  {
    switch(this->m_parser->http_errno)
      {
      case HPE_OK:
      case HPE_PAUSED:
        return http_status_ok;

      case HPE_INVALID_VERSION:
        return http_status_http_version_not_supported;

      case HPE_INVALID_METHOD:
        return http_status_method_not_allowed;

      case HPE_INVALID_TRANSFER_ENCODING:
        return http_status_length_required;

      default:
        return http_status_bad_request;
      }
  }

void
HTTP_Request_Parser::
clear() noexcept
  {
    ::http_parser_init(this->m_parser, HTTP_REQUEST);
    this->m_parser->data = this;

    this->m_headers.clear();
    this->m_payload.clear();

    this->m_hreq = hreq_new;
    this->m_close_after_payload = false;
  }

void
HTTP_Request_Parser::
deallocate() noexcept
  {
    ::rocket::exchange(this->m_headers.raw_host);
    ::rocket::exchange(this->m_headers.raw_userinfo);
    ::rocket::exchange(this->m_headers.raw_path);
    ::rocket::exchange(this->m_headers.raw_query);
    ::rocket::exchange(this->m_headers.headers);
    ::rocket::exchange(this->m_payload);
  }

void
HTTP_Request_Parser::
parse_headers_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hreq >= hreq_headers_done)
      return;

    // Consume incoming data.
    if(data.size() != 0)
      data.discard(::http_parser_execute(this->m_parser, s_settings, data.data(), data.size()));

    // If the caller indicates EOF, then also notify the HTTP parser an EOF.
    if(eof)
      ::http_parser_execute(this->m_parser, s_settings, "", 0);

    // If the parser has been paused, unpause it, so the caller won't see it.
    if(this->m_parser->http_errno == HPE_PAUSED)
      this->m_parser->http_errno = HPE_OK;
  }

void
HTTP_Request_Parser::
parse_payload_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hreq >= hreq_payload_done)
      return;

    if(this->m_hreq != hreq_headers_done)
      POSEIDON_THROW((
          "HTTP request header not parsed yet (state `$1` not valid)"),
          this->m_hreq);

    // Consume incoming data.
    if(data.size() != 0)
      data.discard(::http_parser_execute(this->m_parser, s_settings, data.data(), data.size()));

    // If the caller indicates EOF, then also notify the HTTP parser an EOF.
    if(eof)
      ::http_parser_execute(this->m_parser, s_settings, "", 0);

    // If the parser has been paused, unpause it, so the caller won't see it.
    if(this->m_parser->http_errno == HPE_PAUSED)
      this->m_parser->http_errno = HPE_OK;
  }

}  // namespace poseidon
