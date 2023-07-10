// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_RESPONSE_PARSER_
#define POSEIDON_HTTP_HTTP_RESPONSE_PARSER_

#include "../fwd.hpp"
#include "http_response_headers.hpp"
#include <http_parser.h>
namespace poseidon {

class HTTP_Response_Parser
  {
  private:
    static const ::http_parser_settings s_settings[1];
    ::http_parser m_parser[1];
    HTTP_Response_Headers m_headers;
    linear_buffer m_body;
    bool m_close_after_body;
    bool m_headers_complete;
    bool m_message_complete;

  public:
    // Constructs a parser for incoming responses.
    HTTP_Response_Parser() noexcept
      {
        this->clear();
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(HTTP_Response_Parser);

    // Has an error occurred?
    bool
    error() const noexcept
      { return HTTP_PARSER_ERRNO(this->m_parser) != HPE_OK;  }

    // Clears all fields. This function shall not be called unless the parser is
    // to be reused for another stream.
    void
    clear() noexcept
      {
        ::http_parser_init(this->m_parser, HTTP_RESPONSE);
        this->m_parser->allow_chunked_length = true;
        this->m_parser->data = this;

        this->m_headers.clear();
        this->m_body.clear();
        this->m_close_after_body = false;
        this->m_headers_complete = false;
        this->m_message_complete = false;
      }

    // Parses the response line and headers of an HTTP response from a stream.
    // `data` may be consumed partially, and must be preserved between calls. If
    // `headers_complete()` returns `true` before the call, this function does
    // nothing.
    void
    parse_headers_from_stream(linear_buffer& data, bool eof);

    // Informs the HTTP parser that the current message has no body, e.g. the
    // response to a HEAD or CONNECT request. This cannot be undone.
    void
    set_no_body() noexcept
      {
        ROCKET_ASSERT(this->m_headers_complete);
        this->m_parser->flags |= F_SKIPBODY;
      }

    // Get the parsed headers.
    bool
    should_close_after_body() const noexcept
      { return this->m_close_after_body;  }

    bool
    headers_complete() const noexcept
      { return this->m_headers_complete;  }

    const HTTP_Response_Headers&
    headers() const noexcept
      { return this->m_headers;  }

    HTTP_Response_Headers&
    mut_headers() noexcept
      { return this->m_headers;  }

    // Parses the body of an HTTP response from a stream. `data` may be consumed
    // partially, and must be preserved between calls. If `body_complete()`
    // returns `true` before the call, this function does nothing.
    void
    parse_body_from_stream(linear_buffer& data, bool eof);

    // Get the parsed body.
    bool
    body_complete() const noexcept
      { return this->m_message_complete;  }

    const linear_buffer&
    body() const noexcept
      { return this->m_body;  }

    linear_buffer&
    mut_body() noexcept
      { return this->m_body;  }

    // Clears the current complete message, so the parser can start the next one.
    void
    next_message() noexcept
      {
        ROCKET_ASSERT(this->m_message_complete);
        this->m_headers.clear();
        this->m_body.clear();
        this->m_close_after_body = false;
        this->m_headers_complete = false;
        this->m_message_complete = false;
      }
  };

}  // namespace poseidon
#endif