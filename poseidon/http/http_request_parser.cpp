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
  {{
    // on_message_begin
    nullptr,

    // on_url
    +[](::http_parser* ps, const char* str, size_t len)
      {
        this->m_headers.method = sref(::http_method_str((::http_method) ps->method));
        this->m_headers.uri.append(str, len);
        return 0;
      },

    // on_status
    nullptr,

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
