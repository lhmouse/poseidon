// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_response_parser.hpp"
#include "../base/config_file.hpp"
#include "../static/main_config.hpp"
#include "../utils.hpp"
namespace poseidon {

POSEIDON_VISIBILITY_HIDDEN
const ::http_parser_settings
HTTP_Response_Parser::s_settings[1] =
#define this   static_cast<HTTP_Response_Parser*>(ps->data)
  {{
    // on_message_begin
    nullptr,

    // on_url
    nullptr,

    // on_status
    +[](::http_parser* ps, const char* str, size_t len)
      {
        this->m_headers.status = static_cast<HTTP_Status>(ps->status_code);
        this->m_headers.reason.append(str, len);
        return 0;
      },

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
        // The headers are complete, so halt.
        this->m_hresp = hresp_headers_done;
        this->m_close_after_payload = ::http_should_keep_alive(ps) == 0;
        ps->http_errno = HPE_PAUSED;
        return 0;
      },

    // on_body
    +[](::http_parser* ps, const char* str, size_t len)
      {
        this->m_payload.putn(str, len);
        return 0;
      },

    // on_message_complete
    +[](::http_parser* ps)
      {
        // The message is complete, so halt.
        this->m_hresp = hresp_payload_done;
        ps->http_errno = HPE_PAUSED;
        return 0;
      },

    // on_chunk_header
    nullptr,

    // on_chunk_complete
    nullptr,
  }};
#undef this

HTTP_Response_Parser::
HTTP_Response_Parser()
  {
    ::http_parser_init(this->m_parser, HTTP_RESPONSE);
    this->m_parser->data = this;
    this->m_parser->allow_chunked_length = true;

    auto conf_file = main_config.copy();
    this->m_default_compression_level = static_cast<int>(conf_file.get_integer_opt(
                                &"network.http.default_compression_level", 0, 9).value_or(6));
    this->m_max_content_length = static_cast<uint32_t>(conf_file.get_integer_opt(
                     &"network.http.max_response_content_length", 256, 16777216).value_or(1048576));
  }

HTTP_Response_Parser::
~HTTP_Response_Parser()
  {
  }

void
HTTP_Response_Parser::
clear()
  noexcept
  {
    ::http_parser_init(this->m_parser, HTTP_RESPONSE);
    this->m_parser->data = this;
    this->m_parser->allow_chunked_length = true;

    this->m_headers.clear();
    this->m_payload.clear();

    this->m_hresp = hresp_new;
    this->m_close_after_payload = false;
  }

void
HTTP_Response_Parser::
deallocate()
  noexcept
  {
    ::rocket::exchange(this->m_headers.reason);
    ::rocket::exchange(this->m_headers.headers);
    ::rocket::exchange(this->m_payload);
  }

void
HTTP_Response_Parser::
parse_headers_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hresp >= hresp_headers_done)
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
HTTP_Response_Parser::
parse_payload_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hresp >= hresp_payload_done)
      return;

    if(this->m_hresp != hresp_headers_done)
      POSEIDON_THROW((
          "HTTP response header not parsed yet (state `$1` not valid)"),
          this->m_hresp);

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
