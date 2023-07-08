// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_HTTP_REQUEST_PARSER_
#define POSEIDON_SOCKET_HTTP_REQUEST_PARSER_

#include "../fwd.hpp"
#include "http_request_headers.hpp"
#include <http_parser.h>
namespace poseidon {

class HTTP_Request_Parser
  {
  private:
    static const ::http_parser_settings s_settings[1];
    ::http_parser m_parser[1];
    HTTP_Request_Headers m_headers;
    linear_buffer m_body;
    bool m_close_after_body = false;
    bool m_headers_complete = false;
    bool m_message_complete = false;

  public:
    // Constructs a parser for incoming requests.
    HTTP_Request_Parser() noexcept
      {
        this->m_parser->data = this;
        ::http_parser_init(this->m_parser, HTTP_REQUEST);
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(HTTP_Request_Parser);

    // Has an error occurred?
    bool
    error() const noexcept
      { return HTTP_PARSER_ERRNO(this->m_parser) != HPE_OK;  }

    // Translates the error code to an HTTP status code.
    ROCKET_PURE
    uint32_t
    http_status_from_error() const noexcept;

    // Clears all fields. This function shall not be called unless the parser is
    // to be reused for another stream.
    void
    clear() noexcept
      {
        ::http_parser_init(this->m_parser, HTTP_REQUEST);
        this->m_headers.clear();
        this->m_body.clear();
        this->m_close_after_body = false;
        this->m_headers_complete = false;
        this->m_message_complete = false;
      }

    // Parses the request line and headers of an HTTP request from a stream.
    // `data` may be consumed partially, and must be preserved between calls. If
    // `headers_complete()` returns `true` before the call, this function does
    // nothing.
    void
    parse_headers_from_stream(linear_buffer& data, bool eof);

    // Get the parsed headers.
    bool
    should_close_after_body() const noexcept
      { return this->m_close_after_body;  }

    bool
    headers_complete() const noexcept
      { return this->m_headers_complete;  }

    const HTTP_Request_Headers&
    headers() const noexcept
      { return this->m_headers;  }

    HTTP_Request_Headers&
    mut_headers() noexcept
      { return this->m_headers;  }

    // Parses the body of an HTTP request from a stream. `data` may be consumed
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
