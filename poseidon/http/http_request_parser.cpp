// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_request_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

constexpr
const ::http_parser_settings
HTTP_Request_Parser::s_settings[1] =
#define this   static_cast<HTTP_Request_Parser*>(ps->data)
#define temp_uri                m_headers.uri_query
#define has_temp_header_value   m_headers.is_ssl
#define temp_header_value       m_headers.uri_path
  {{
    // on_message_begin
    nullptr,

    // on_url
    +[](::http_parser* ps, const char* str, size_t len)
      {
        this->m_headers.method = ::http_method_str((::http_method) ps->method);
        this->temp_uri.append(str, len);
        return 0;
      },

    // on_status
    nullptr,

    // on_header_field
    +[](::http_parser* ps, const char* str, size_t len)
      {
        if(this->m_headers.headers.empty())
          this->m_headers.headers.emplace_back();

        if(this->has_temp_header_value) {
          // If a previous value exists, the header is now complete, so parse it.
          auto& value = this->m_headers.headers.mut_back().second;
          size_t plen = value.parse(this->temp_header_value.data(), this->temp_header_value.size());

          // If the value doesn't match a special format, accept it as a string.
          if(plen != this->temp_header_value.size())
            value = ::std::move(this->temp_header_value);

          // Prepare the next header.
          this->m_headers.headers.emplace_back();
          this->has_temp_header_value = false;
          this->temp_header_value.clear();
        }

        // Append the header name to the last key, as this callback might be
        // invoked repeatedly.
        this->m_headers.headers.mut_back().first.append(str, len);
        return 0;
      },

    // on_header_value
    +[](::http_parser* ps, const char* str, size_t len)
      {
        this->has_temp_header_value = true;
        this->temp_header_value.append(str, len);
        return 0;
      },

    // on_headers_complete
    +[](::http_parser* ps)
      {
        if(this->has_temp_header_value) {
          // If a previous value exists, the header is now complete, so parse it.
          auto& value = this->m_headers.headers.mut_back().second;
          size_t plen = value.parse(this->temp_header_value.data(), this->temp_header_value.size());

          // If the value doesn't match a special format, accept it as a string.
          if(plen != this->temp_header_value.size())
            value = ::std::move(this->temp_header_value);
        }

        // Parse the URI.
        cow_string uri = ::std::move(this->temp_uri);
        ::http_parser_url uri_hp = { };

        if((uri.size() > UINT16_MAX) || (::http_parser_parse_url(uri.data(), uri.size(), ps->method == HTTP_CONNECT, &uri_hp))) {
          ps->http_errno = HPE_INVALID_URL;
          return 0;
        }

        this->temp_uri.clear();
        this->has_temp_header_value = false;
        this->temp_header_value.clear();

        if(uri_hp.field_set & (1U << UF_SCHEMA)) {
          // The scheme shall be `http://` or `https://`.
          if(::rocket::ascii_ci_equal(uri.data() + uri_hp.field_data[UF_SCHEMA].off, uri_hp.field_data[UF_SCHEMA].len, "http", 4)) {
            this->m_headers.is_proxy = true;
            this->m_headers.is_ssl = false;
          }
          else if(::rocket::ascii_ci_equal(uri.data() + uri_hp.field_data[UF_SCHEMA].off, uri_hp.field_data[UF_SCHEMA].len, "https", 5)) {
            this->m_headers.is_proxy = true;
            this->m_headers.is_ssl = true;
          }
          else {
            ps->http_errno = HPE_INVALID_URL;
            return 2;
          }
        }

        if(uri_hp.field_set & (1U << UF_HOST)) {
          // Use the host name in the request URI and ignore the `Host:` header.
          this->m_headers.is_proxy = true;
          this->m_headers.uri_host.assign(uri, uri_hp.field_data[UF_HOST].off, uri_hp.field_data[UF_HOST].len);
        }
        else {
          // Get the `Host:` header. Multiple `Host:` headers are not allowed.
          bool host_header_found = false;

          for(const auto& hpair : this->m_headers.headers)
            if(ascii_ci_equal(hpair.first, sref("Host"))) {
              if(hpair.second.is_string() && !host_header_found) {
                host_header_found = true;
                this->m_headers.uri_host = hpair.second.as_string();
              }
              else {
                ps->http_errno = HPE_INVALID_URL;
                return 2;
              }
            }

          if(!host_header_found) {
            ps->http_errno = HPE_INVALID_URL;
            return 2;
          }
        }

        this->m_headers.uri_port = uri_hp.port;
        this->m_headers.uri_userinfo.assign(uri, uri_hp.field_data[UF_USERINFO].off, uri_hp.field_data[UF_USERINFO].len);
        this->m_headers.uri_path.assign(uri, uri_hp.field_data[UF_PATH].off, uri_hp.field_data[UF_PATH].len);
        this->m_headers.uri_query.assign(uri, uri_hp.field_data[UF_QUERY].off, uri_hp.field_data[UF_QUERY].len);

        // The headers are complete, so halt.
        this->m_hreq = hreq_headers_done;
        this->m_close_after_payload = ::http_should_keep_alive(ps) == 0;
        ps->http_errno = HPE_PAUSED;
        return 0;
      },

    // on_body
    +[](::http_parser* ps, const char* str, size_t len)
      {
        if(is_none_of(ps->method, { HTTP_GET, HTTP_HEAD, HTTP_DELETE, HTTP_CONNECT })) {
          // For the methods above, the request payload is discarded.
          this->m_payload.putn(str, len);
        }
        return 0;
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
#undef temp_uri
#undef has_temp_header_value
#undef temp_header_value

HTTP_Request_Parser::
~HTTP_Request_Parser()
  {
  }

uint32_t
HTTP_Request_Parser::
http_status_from_error() const noexcept
  {
    switch(this->m_parser->http_errno) {
      case HPE_OK:
      case HPE_PAUSED:
        return HTTP_STATUS_OK;

      case HPE_INVALID_METHOD:
        return HTTP_STATUS_METHOD_NOT_ALLOWED;

      case HPE_INVALID_VERSION:
        return HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED;

      case HPE_HEADER_OVERFLOW:
        return HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE;

      case HPE_INVALID_TRANSFER_ENCODING:
        return HTTP_STATUS_LENGTH_REQUIRED;

      default:
        return HTTP_STATUS_BAD_REQUEST;
    }
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
