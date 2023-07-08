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
        if(this->m_headers.reason.empty()) {
          // The reason string is very likely a predefined one, so don't bother
          // allocating storage for it.
          auto stat_sh = sref(::http_status_str((::http_status) ps->status_code));
          if(xmemeq(stat_sh.data(), stat_sh.size(), str, len))
            this->m_headers.reason = stat_sh;
          else
            this->m_headers.reason.assign(str, len);
        }
        else {
          this->m_headers.reason.append(str, len);
        }
        return 0;
      },

    // on_header_field
    +[](::http_parser* ps, const char* str, size_t len)
      {
        if(this->m_headers.headers.empty())
          this->m_headers.headers.emplace_back();

        if(this->m_body.size() != 0) {
          // If `m_body` is not empty, a previous header is now complete.
          const char* value_str = this->m_body.data() + 1;
          ROCKET_ASSERT(value_str[-1] == '\n');  // magic character
          size_t value_len = this->m_body.size() - 1;

          auto& value = this->m_headers.headers.mut_back().second;
          if(value.parse(value_str, value_len) != value_len)
            value.set_string(cow_string(value_str, value_len));

          // Create the next header.
          this->m_headers.headers.emplace_back();
          this->m_body.clear();
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
        // received. This makes `m_body` non-empty.
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
          // If `m_body` is not empty, a previous header is now complete.
          const char* value_str = this->m_body.data() + 1;
          ROCKET_ASSERT(value_str[-1] == '\n');  // magic character
          size_t value_len = this->m_body.size() - 1;

          auto& value = this->m_headers.headers.mut_back().second;
          if(value.parse(value_str, value_len) != value_len)
            value.set_string(cow_string(value_str, value_len));

          // Finish abuse of `m_body`.
          this->m_body.clear();
        }

        // The headers are complete, so halt.
        this->m_close_after_body = ::http_should_keep_alive(ps) == 0;
        this->m_headers_complete = true;
        ps->http_errno = HPE_PAUSED;
        return 0;
      },

    // on_body
    +[](::http_parser* ps, const char* str, size_t len)
      {
        this->m_body.putn(str, len);
        return 0;
      },

    // on_message_complete
    +[](::http_parser* ps)
      {
        // The message is complete, so halt.
        this->m_message_complete = true;
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
    if(this->m_headers_complete)
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
parse_body_from_stream(linear_buffer& data, bool eof)
  {
    if(this->m_message_complete)
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

}  // namespace poseidon
