// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/http_response_parser.hpp"
#include "../../http/http_method.hpp"
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
    nullptr,

    // on_url
    nullptr,

    // on_status
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_header_size + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_header_size + len, this->m_max_header_length);

        this->m_headers.reason.append(str, len);
        this->m_header_size += static_cast<uint32_t>(len);
        return 0;
      },

    // on_method
    nullptr,

    // on_version
    nullptr,

    // on_header_field
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_hresp >= hresp_headers_done)  // chunk trailer
          return 0;

        if(this->m_header_size + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_header_size + len, this->m_max_header_length);

        this->m_stash.append(str, len);
        this->m_header_size += static_cast<uint32_t>(len);
        return 0;
      },

    // on_header_value
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_hresp >= hresp_headers_done)  // chunk trailer
          return 0;

        if(this->m_header_size + len > this->m_max_header_length)
          POSEIDON_THROW((
              "HTTP header length limit exceeded: `$1` > `$2`"),
              this->m_header_size + len, this->m_max_header_length);

        this->m_stash.append(str, len);
        this->m_header_size += static_cast<uint32_t>(len);
        return 0;
      },

    // on_chunk_extension_name
    nullptr,

    // on_chunk_extension_value
    nullptr,

    // on_headers_complete
    +[](::llhttp_t* ps) -> int
      {
        if(this->m_hresp >= hresp_headers_done)  // chunk trailer
          return 0;

        this->m_hresp = hresp_headers_done;
        this->m_close_after_content = ::llhttp_should_keep_alive(ps) == 0;
        this->m_stash.clear();
        return HPE_PAUSED;
      },

    // on_body
    +[](::llhttp_t* ps, const char* str, size_t len) -> int
      {
        if(this->m_content.size() + len > this->m_max_content_length)
          POSEIDON_THROW((
              "HTTP content length limit exceeded: `$1` > `$2`"),
              this->m_content.size() + len, this->m_max_content_length);

        this->m_content.putn(str, len);
        return 0;
      },

    // on_message_complete
    +[](::llhttp_t* ps) -> int
      {
        this->m_hresp = hresp_content_done;
        return HPE_PAUSED;
      },

    // on_protocol_complete
    nullptr,

    // on_url_complete
    nullptr,

    // on_status_complete
    +[](::llhttp_t* ps) -> int
      {
        this->m_headers.status = static_cast<HTTP_Status>(ps->status_code);
        return 0;
      },

    // on_method_complete
    nullptr,

    // on_version_complete
    nullptr,

    // on_header_field_complete
    +[](::llhttp_t* ps) -> int
      {
        if(this->m_hresp >= hresp_headers_done)  // chunk trailer
          return 0;

        this->m_headers.headers.emplace_back(move(this->m_stash), &"");
        this->m_stash.clear();
        return 0;
      },

    // on_header_value_complete
    +[](::llhttp_t* ps) -> int
      {
        if(this->m_hresp >= hresp_headers_done)  // chunk trailer
          return 0;

        this->m_headers.headers.mut_back().second = move(this->m_stash);
        this->m_stash.clear();
        this->m_header_size += 32;
        return 0;
      },

    // on_chunk_extension_name_complete
    nullptr,

    // on_chunk_extension_value_complete
    nullptr,

    // on_chunk_header
    nullptr,

    // on_chunk_complete
    nullptr,

    // on_reset
    nullptr,
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
next_message() noexcept
  {
    this->m_headers.clear();
    this->m_content.clear();
    this->m_stash.clear();
    this->m_header_size = 0;
    this->m_hresp = hresp_new;
    this->m_close_after_content = false;
  }

void
HTTP_Response_Parser::
clear() noexcept
  {
    ::llhttp_reset(&(this->m_llhttp_parser));
    this->next_message();
  }

void
HTTP_Response_Parser::
deallocate() noexcept
  {
    ::asteria::exchange(this->m_headers.reason);
    ::asteria::exchange(this->m_headers.headers);
    ::asteria::exchange(this->m_content);
    ::asteria::exchange(this->m_stash);
  }

void
HTTP_Response_Parser::
parse_headers_from_stream(HTTP_Method method, linear_buffer& data, bool eof)
  {
    if(this->m_hresp >= hresp_headers_done)
      return;

    if(method == http_method_HEAD)
      this->m_llhttp_parser.flags |= F_SKIPBODY;

    this->m_llhttp_parser.error_pos = nullptr;
    ::llhttp_errno_t err = ::llhttp_execute(&(this->m_llhttp_parser), data.data(), data.size());

    // If `err` is `HPE_PAUSED`, we should consume parsed bytes; otherwise it
    // means either all bytes have been consumed or there is an error, and we
    // discard all input data.
    if(err == HPE_PAUSED)
      data.discard(static_cast<size_t>(this->m_llhttp_parser.error_pos - data.data()));
    else
      data.clear();

    if((err == HPE_OK) && eof) {
      // `llhttp_finish()` doesn't set `m_llhttp_parser.error` so it has to be
      // copied explicitly.
      err = ::llhttp_finish(&(this->m_llhttp_parser));
      if(err != HPE_OK)
        this->m_llhttp_parser.error = err;
    }

    // `HPE_PAUSED` means the end of a message has been reached, which should
    // not passed to the caller.
    if(this->m_llhttp_parser.error == HPE_PAUSED)
      this->m_llhttp_parser.error = HPE_OK;
  }

void
HTTP_Response_Parser::
parse_content_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_hresp >= hresp_content_done)
      return;

    if(this->m_hresp != hresp_headers_done)
      POSEIDON_THROW((
          "HTTP response header not parsed yet (state `$1` not valid)"),
          this->m_hresp);

    this->m_llhttp_parser.error_pos = nullptr;
    ::llhttp_errno_t err = ::llhttp_execute(&(this->m_llhttp_parser), data.data(), data.size());

    // If `err` is `HPE_PAUSED`, we should consume parsed bytes; otherwise it
    // means either all bytes have been consumed or there is an error, and we
    // discard all input data.
    if(err == HPE_PAUSED)
      data.discard(static_cast<size_t>(this->m_llhttp_parser.error_pos - data.data()));
    else
      data.clear();

    if((err == HPE_OK) && eof) {
      // `llhttp_finish()` doesn't set `m_llhttp_parser.error` so it has to be
      // copied explicitly.
      err = ::llhttp_finish(&(this->m_llhttp_parser));
      if(err != HPE_OK)
        this->m_llhttp_parser.error = err;
    }

    // `HPE_PAUSED` means the end of a message has been reached, which should
    // not passed to the caller.
    if(this->m_llhttp_parser.error == HPE_PAUSED)
      this->m_llhttp_parser.error = HPE_OK;
  }

}  // namespace poseidon
