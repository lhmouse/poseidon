// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/http_request_parser.hpp"
#include "../../base/config_file.hpp"
#include "../../static/main_config.hpp"
#include "../../utils.hpp"
namespace poseidon {

POSEIDON_VISIBILITY_HIDDEN
const ::llhttp_settings_t
HTTP_Request_Parser::s_llhttp_settings =
#define this   static_cast<HTTP_Request_Parser*>(ps->data)
  {
    // on_message_begin
    nullptr,

    // on_protocol
    nullptr,

    // on_url
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        // Append the substring to `m_headers.raw_query` which is abused as
        // temporary storage.
        if(this->m_headers.estimate_size() + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_headers.estimate_size() + len, this->m_max_header_length);

        this->m_headers.raw_query.append(str, len);
        return 0;
      },

    // on_status
    nullptr,

    // on_method
    nullptr,

    // on_version
    nullptr,

    // on_header_field
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        // If this notification is received when no header exists, or a previous
        // header value has been accepted, then a new header starts.
        if(this->m_headers.headers.empty() || !this->m_headers.headers.back().second.is_null())
          this->m_headers.headers.emplace_back();

        // Append the header name to the last key, as this callback might be
        // invoked repeatedly.
        if(this->m_headers.estimate_size() + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_headers.estimate_size() + len, this->m_max_header_length);

        this->m_headers.headers.mut_back().first.mut_str().append(str, len);
        return 0;
      },

    // on_header_value
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        // Append the header value, as this callback might be invoked repeatedly.
        if(this->m_headers.estimate_size() + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_headers.estimate_size() + len, this->m_max_header_length);

        cow_string value = this->m_headers.headers.back().second.as_string();
        value.append(str, len);
        this->m_headers.headers.mut_back().second = move(value);
        return 0;
      },

    // on_chunk_extension_name
    nullptr,

    // on_chunk_extension_value
    nullptr,

    // on_headers_complete

    // on_body
    // on_message_complete
    // on_protocol_complete
    // on_url_complete
    // on_status_complete
    // on_method_complete
    // on_version_complete
    // on_header_field_complete
    // on_header_value_complete
    // on_chunk_extension_name_complete
    // on_chunk_extension_value_complete
    // on_chunk_header
    // on_chunk_complete
    // on_reset
  };
#undef this

HTTP_Request_Parser::
HTTP_Request_Parser()
  {
    ::llhttp_init(&(this->m_llhttp_parser), HTTP_REQUEST, &(s_llhttp_settings));
    this->m_llhttp_parser.data = this;

    auto conf_file = main_config.copy();
    this->m_default_compression_level = static_cast<int>(conf_file.get_integer_opt(
        &"network.http.default_compression_level", 0, 9).value_or(6));
    this->m_max_header_length = static_cast<uint32_t>(conf_file.get_integer_opt(
        &"network.http.max_header_length", 256, 16777216).value_or(262144));
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
    switch(this->m_llhttp_parser.error)
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
    ::llhttp_init(&(this->m_llhttp_parser), HTTP_REQUEST, &(s_llhttp_settings));
    this->m_llhttp_parser.data = this;

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
    ::asteria::exchange(this->m_headers.raw_host);
    ::asteria::exchange(this->m_headers.raw_userinfo);
    ::asteria::exchange(this->m_headers.raw_path);
    ::asteria::exchange(this->m_headers.raw_query);
    ::asteria::exchange(this->m_headers.headers);
    ::asteria::exchange(this->m_payload);
  }

void
HTTP_Request_Parser::
parse_headers_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hreq >= hreq_headers_done)
      return;

    this->m_llhttp_parser.error_pos = data.data();
    ::llhttp_execute(&(this->m_llhttp_parser), data.data(), data.size());

    if(eof)
      ::llhttp_finish(&(this->m_llhttp_parser));

    if(this->m_llhttp_parser.error == HPE_PAUSED) {
      // This means the end of a message has been reached. Consume parsed bytes,
      // and reset the error so the caller won't see it.
      data.discard(static_cast<size_t>(this->m_llhttp_parser.error_pos - data.data()));
      this->m_llhttp_parser.error = HPE_OK;
    }
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

    this->m_llhttp_parser.error_pos = data.data();
    ::llhttp_execute(&(this->m_llhttp_parser), data.data(), data.size());

    if(eof)
      ::llhttp_finish(&(this->m_llhttp_parser));

    if(this->m_llhttp_parser.error == HPE_PAUSED) {
      // This means the end of a message has been reached. Consume parsed bytes,
      // and reset the error so the caller won't see it.
      data.discard(static_cast<size_t>(this->m_llhttp_parser.error_pos - data.data()));
      this->m_llhttp_parser.error = HPE_OK;
    }
  }

}  // namespace poseidon
