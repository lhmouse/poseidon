// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

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

        // Parse the request URI.
        cow_string uri;
        uri.swap(this->m_headers.raw_host);

        if((uri.size() >= 7) && ::rocket::ascii_ci_equal(uri.c_str(), 7, "http://", 7)) {
          this->m_headers.port = 80;
          this->m_headers.is_proxy = true;
          this->m_headers.is_ssl = false;
          uri.erase(0, 7);
        }
        else if((uri.size() >= 8) && ::rocket::ascii_ci_equal(uri.c_str(), 8, "https://", 8)) {
          this->m_headers.port = 443;
          this->m_headers.is_proxy = true;
          this->m_headers.is_ssl = true;
          uri.erase(0, 8);
        }
        else if(uri.find("://") != cow_string::npos) {
          ps->http_errno = HPE_INVALID_URL;
          return 2;
        }

        Network_Reference ref;
        if(this->m_headers.is_proxy) {
          // Userinfo is allowed.
          size_t t = uri.find_of("@/");
          if((t != cow_string::npos) && (uri[t] == '@')) {
            this->m_headers.raw_userinfo.assign(uri, 0, t);
            uri.erase(0, t + 1);
          }

          // Hostname is required.
          if(parse_network_reference(ref, uri) != uri.size()) {
            ps->http_errno = HPE_INVALID_HOST;
            return 2;
          }

          this->m_headers.raw_host.assign(ref.host.p, ref.host.n);
        }
        else {
          // Neither userinfo nor hostname is allowed.
          if(uri[0] != '/') {
            ps->http_errno = HPE_INVALID_PATH;
            return 2;
          }

          uri.insert(0, 1, 'a');
          if(parse_network_reference(ref, uri) != uri.size()) {
            ps->http_errno = HPE_INVALID_HOST;
            return 2;
          }

          // Get the `Host:` header. Multiple `Host:` headers are not allowed.
          uint32_t count = 0;
          for(const auto& hr : this->m_headers.headers)
            if(hr.first == "Host") {
              count ++;
              if(count == 1)
                this->m_headers.raw_host = hr.second.as_string();
              else
                break;
            }

          if(count != 1) {
            ps->http_errno = HPE_INVALID_URL;
            return 2;
          }
        }

        if(ref.port.n != 0)
          this->m_headers.port = ref.port_num;

        if(ref.path.n != 0)
          this->m_headers.raw_path.assign(ref.path.p, ref.path.n);
        else
          this->m_headers.raw_path = &"/";

        if(ref.query.n != 0)
          this->m_headers.raw_query.assign(ref.query.p, ref.query.n);

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

    auto conf_file = main_config.copy();
    this->m_default_compression_level = static_cast<int>(conf_file.get_integer_opt(
                                &"network.http.default_compression_level", 0, 9).value_or(6));
    this->m_max_content_length = static_cast<uint32_t>(conf_file.get_integer_opt(
                     &"network.http.max_request_content_length", 256, 16777216).value_or(1048576));
  }

HTTP_Request_Parser::
~HTTP_Request_Parser()
  {
  }

HTTP_Status
HTTP_Request_Parser::
http_status_from_error()
  const noexcept
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
clear()
  noexcept
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
deallocate()
  noexcept
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
