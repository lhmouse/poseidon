// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_RESPONSE_PARSER_
#define POSEIDON_HTTP_HTTP_RESPONSE_PARSER_

#include "../fwd.hpp"
#include "http_response_headers.hpp"
#include <llhttp.h>
namespace poseidon {

class HTTP_Response_Parser
  {
  private:
    int m_default_compression_level;
    uint32_t m_max_header_length;
    uint32_t m_max_content_length;

    static const ::llhttp_settings_t s_llhttp_settings;
    ::llhttp_t m_llhttp_parser;
    HTTP_Response_Headers m_headers;
    linear_buffer m_content;

    enum HRESP_State : uint8_t
      {
        hresp_new           = 0,
        hresp_headers_done  = 1,
        hresp_content_done  = 2,
      };

    cow_string m_stash;
    uint32_t m_header_size = 0;
    HRESP_State m_hresp = hresp_new;
    bool m_close_after_content = false;

  public:
    // Constructs a parser for incoming responses.
    HTTP_Response_Parser();

  public:
    HTTP_Response_Parser(const HTTP_Response_Parser&) = delete;
    HTTP_Response_Parser& operator=(const HTTP_Response_Parser&) & = delete;
    ~HTTP_Response_Parser();

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

    // Parses the response line and headers of an HTTP response from a stream.
    // `data` may be consumed partially, and must be preserved between calls. If
    // `headers_complete()` returns `true` before the call, this function does
    // nothing.
    void
    parse_headers_from_stream(HTTP_Method method, linear_buffer& data, bool eof);

    // Get the parsed headers.
    bool
    should_close_after_content() const noexcept
      { return this->m_close_after_content;  }

    bool
    headers_complete() const noexcept
      { return this->m_hresp >= hresp_headers_done;  }

    const HTTP_Response_Headers&
    headers() const noexcept
      { return this->m_headers;  }

    HTTP_Response_Headers&
    mut_headers() noexcept
      { return this->m_headers;  }

    // Parses the content of an HTTP response from a stream. `data` may be consumed
    // partially, and must be preserved between calls. If `content_complete()`
    // returns `true` before the call, this function does nothing.
    void
    parse_content_from_stream(linear_buffer& data, bool eof);

    // Get the parsed content.
    bool
    content_complete() const noexcept
      { return this->m_hresp >= hresp_content_done;  }

    const linear_buffer&
    content() const noexcept
      { return this->m_content;  }

    linear_buffer&
    mut_content() noexcept
      { return this->m_content;  }
  };

}  // namespace poseidon
#endif
