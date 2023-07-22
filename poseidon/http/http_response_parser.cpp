// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_response_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

constexpr
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
        this->m_headers.status = ps->status_code;
        this->m_headers.reason.append(str, len);
        return 0;
      },

    // on_header_field
    +[](::http_parser* ps, const char* str, size_t len)
      {
        if(this->m_headers.headers.empty())
          this->m_headers.headers.emplace_back();

        if(this->m_payload.size() != 0) {
          // If `m_payload` is not empty, a previous header is now complete.
          const char* value_str = this->m_payload.data() + 1;
          ROCKET_ASSERT(value_str[-1] == '\n');  // magic character
          size_t value_len = this->m_payload.size() - 1;

          auto& value = this->m_headers.headers.mut_back().second;
          if(value.parse(value_str, value_len) != value_len)
            value.set_string(cow_string(value_str, value_len));

          // Create the next header.
          this->m_headers.headers.emplace_back();
          this->m_payload.clear();
        }

        // Append the header name to the last key, as this callback might be
        // invoked repeatedly.
        this->m_headers.headers.mut_back().first.append(str, len);
        return 0;
      },

    // on_header_value
    +[](::http_parser* ps, const char* str, size_t len)
      {
        // Add a magic character to indicate that a part of the value has been
        // received. This makes `m_payload` non-empty.
        if(this->m_payload.empty())
           this->m_payload.putc('\n');

        // Abuse `m_payload` to store the header value.
        this->m_payload.putn(str, len);
        return 0;
      },

    // on_headers_complete
    +[](::http_parser* ps)
      {
        if(this->m_payload.size() != 0) {
          // If `m_payload` is not empty, a previous header is now complete.
          const char* value_str = this->m_payload.data() + 1;
          ROCKET_ASSERT(value_str[-1] == '\n');  // magic character
          size_t value_len = this->m_payload.size() - 1;

          auto& value = this->m_headers.headers.mut_back().second;
          if(value.parse(value_str, value_len) != value_len)
            value.set_string(cow_string(value_str, value_len));

          // Finish abuse of `m_payload`.
          this->m_payload.clear();
        }

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
~HTTP_Response_Parser()
  {
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
