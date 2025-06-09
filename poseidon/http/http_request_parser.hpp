// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_REQUEST_PARSER_
#define POSEIDON_HTTP_HTTP_REQUEST_PARSER_

#include "../fwd.hpp"
#include "http_request_headers.hpp"
#include <http_parser.h>
namespace poseidon {

class HTTP_Request_Parser
  {
  private:
    int m_default_compression_level = 6;
    uint32_t m_max_content_length = 1048576;

    static const ::http_parser_settings s_settings[1];
    ::http_parser m_parser[1];
    HTTP_Request_Headers m_headers;
    linear_buffer m_payload;

    enum HREQ_State : uint8_t
      {
        hreq_new           = 0,
        hreq_headers_done  = 1,
        hreq_payload_done  = 2,
      };

    HREQ_State m_hreq = hreq_new;
    bool m_close_after_payload = false;

  public:
    // Constructs a parser for incoming requests.
    HTTP_Request_Parser();

  public:
    HTTP_Request_Parser(const HTTP_Request_Parser&) = delete;
    HTTP_Request_Parser& operator=(const HTTP_Request_Parser&) & = delete;
    ~HTTP_Request_Parser();

    // Get configuration values.
    int
    default_compression_level() const noexcept
      { return this->m_default_compression_level;  }

    uint32_t
    max_content_length() const noexcept
      { return this->m_max_content_length;  }

    // Has an error occurred?
    bool
    error() const noexcept
      { return HTTP_PARSER_ERRNO(this->m_parser) != HPE_OK;  }

    // Translates the error code to an HTTP status code.
    ROCKET_PURE
    HTTP_Status
    http_status_from_error() const noexcept;

    // Clears all fields. This function shall not be called unless the parser is
    // to be reused for another stream.
    void
    clear() noexcept;

    // Deallocate dynamic memory, if any.
    void
    deallocate() noexcept;

    // Parses the request line and headers of an HTTP request from a stream.
    // `data` may be consumed partially, and must be preserved between calls. If
    // `headers_complete()` returns `true` before the call, this function does
    // nothing.
    void
    parse_headers_from_stream(linear_buffer& data, bool eof);

    // Get the parsed headers.
    bool
    should_close_after_payload() const noexcept
      { return this->m_close_after_payload;  }

    bool
    headers_complete() const noexcept
      { return this->m_hreq >= hreq_headers_done;  }

    const HTTP_Request_Headers&
    headers() const noexcept
      { return this->m_headers;  }

    HTTP_Request_Headers&
    mut_headers() noexcept
      { return this->m_headers;  }

    // Parses the payload of an HTTP request from a stream. `data` may be consumed
    // partially, and must be preserved between calls. If `payload_complete()`
    // returns `true` before the call, this function does nothing.
    void
    parse_payload_from_stream(linear_buffer& data, bool eof);

    // Get the parsed payload.
    bool
    payload_complete() const noexcept
      { return this->m_hreq >= hreq_payload_done;  }

    const linear_buffer&
    payload() const noexcept
      { return this->m_payload;  }

    linear_buffer&
    mut_payload() noexcept
      { return this->m_payload;  }

    // Clears the current complete message, so the parser can start the next one.
    void
    next_message() noexcept
      {
        ROCKET_ASSERT(this->m_hreq >= hreq_payload_done);

        this->m_headers.clear();
        this->m_payload.clear();
        this->m_hreq = hreq_new;
        this->m_close_after_payload = false;
      }
  };

}  // namespace poseidon
#endif
