// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_REQUEST_PARSER_
#define POSEIDON_HTTP_HTTP_REQUEST_PARSER_

#include "../fwd.hpp"
#include "http_request_headers.hpp"
#include <llhttp.h>
namespace poseidon {

class HTTP_Request_Parser
  {
  private:
    int m_default_compression_level;
    uint32_t m_max_header_length;
    uint32_t m_max_content_length;

    static const ::llhttp_settings_t s_llhttp_settings;
    ::llhttp_t m_llhttp_parser;
    HTTP_Request_Headers m_headers;
    linear_buffer m_content;

    enum HREQ_State : uint8_t
      {
        hreq_new           = 0,
        hreq_headers_done  = 1,
        hreq_content_done  = 2,
      };

    cow_string m_stash;
    uint32_t m_header_size = 0;
    HREQ_State m_hreq = hreq_new;
    bool m_host_set = false;
    bool m_close_after_content = false;

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
    max_header_length() const noexcept
      { return this->m_max_header_length;  }

    uint32_t
    max_content_length() const noexcept
      { return this->m_max_content_length;  }

    // Has an error occurred?
    bool
    error() const noexcept
      { return this->m_llhttp_parser.error != HPE_OK;  }

    // Translates the error code to an HTTP status code.
    ASTERIA_PURE
    HTTP_Status
    http_status_from_error() const noexcept;

    // Clears the current complete message, so the parser can start the next one.
    void
    next_message() noexcept;

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
    should_close_after_content() const noexcept
      { return this->m_close_after_content;  }

    bool
    headers_complete() const noexcept
      { return this->m_hreq >= hreq_headers_done;  }

    const HTTP_Request_Headers&
    headers() const noexcept
      { return this->m_headers;  }

    HTTP_Request_Headers&
    mut_headers() noexcept
      { return this->m_headers;  }

    // Parses the content of an HTTP request from a stream. `data` may be consumed
    // partially, and must be preserved between calls. If `content_complete()`
    // returns `true` before the call, this function does nothing.
    void
    parse_content_from_stream(linear_buffer& data, bool eof);

    // Get the parsed content.
    bool
    content_complete() const noexcept
      { return this->m_hreq >= hreq_content_done;  }

    const linear_buffer&
    content() const noexcept
      { return this->m_content;  }

    linear_buffer&
    mut_content() noexcept
      { return this->m_content;  }
  };

}  // namespace poseidon
#endif
