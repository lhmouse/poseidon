// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/http_response_parser.hpp"
#include "../../base/config_file.hpp"
#include "../../static/main_config.hpp"
#include "../../utils.hpp"
namespace poseidon {

POSEIDON_VISIBILITY_HIDDEN
const ::llhttp_settings_t
HTTP_Response_Parser::s_llhttp_settings =
#define this   static_cast<HTTP_Response_Parser*>(ps->data)
  {
    // on_message_begin
    nullptr,

    // on_protocol
    // on_url
    // on_status
    // on_method
    // on_version
    // on_header_field
    // on_header_value
    // on_chunk_extension_name
    // on_chunk_extension_value
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

HTTP_Response_Parser::
HTTP_Response_Parser()
  {
    ::llhttp_init(&(this->m_llhttp_parser), HTTP_RESPONSE, &(s_llhttp_settings));
    this->m_llhttp_parser.data = this;
    ::llhttp_set_lenient_chunked_length(&(this->m_llhttp_parser), true);

    auto conf_file = main_config.copy();
    this->m_default_compression_level = static_cast<int>(conf_file.get_integer_opt(
        &"network.http.default_compression_level", 0, 9).value_or(6));
    this->m_max_header_length = static_cast<uint32_t>(conf_file.get_integer_opt(
        &"network.http.max_header_length", 256, 16777216).value_or(262144));
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
    ::llhttp_init(&(this->m_llhttp_parser), HTTP_RESPONSE, &(s_llhttp_settings));
    this->m_llhttp_parser.data = this;
    ::llhttp_set_lenient_chunked_length(&(this->m_llhttp_parser), true);

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
    ::asteria::exchange(this->m_headers.reason);
    ::asteria::exchange(this->m_headers.headers);
    ::asteria::exchange(this->m_payload);
  }

void
HTTP_Response_Parser::
parse_headers_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hresp >= hresp_headers_done)
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
HTTP_Response_Parser::
parse_payload_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hresp >= hresp_payload_done)
      return;

    if(this->m_hresp != hresp_headers_done)
      POSEIDON_THROW((
          "HTTP response header not parsed yet (state `$1` not valid)"),
          this->m_hresp);

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
