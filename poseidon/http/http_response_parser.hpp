// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_RESPONSE_PARSER_
#define POSEIDON_HTTP_HTTP_RESPONSE_PARSER_

#include "../fwd.hpp"
#include "http_response_headers.hpp"
#include <http_parser.h>
namespace poseidon {

class HTTP_Response_Parser
  {
  private:
    int m_default_compression_level = 6;
    uint32_t m_max_content_length = 1048576;

    static const ::http_parser_settings s_settings[1];
    ::http_parser m_parser[1];
    HTTP_Response_Headers m_headers;
    linear_buffer m_payload;

    enum HRESP_State : uint8_t
      {
        hresp_new           = 0,
        hresp_headers_done  = 1,
        hresp_payload_done  = 2,
      };

    HRESP_State m_hresp = hresp_new;
    bool m_close_after_payload = false;
    char m_reserved_1;
    char m_reserved_2;

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
    max_content_length() const noexcept
      { return this->m_max_content_length;  }

    // Has an error occurred?
    bool
    error() const noexcept
      { return HTTP_PARSER_ERRNO(this->m_parser) != HPE_OK;  }

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
    parse_headers_from_stream(linear_buffer& data, bool eof);

    // Informs the HTTP parser that the current message has no payload, e.g. the
    // response to a HEAD or CONNECT request. This cannot be undone.
    void
    set_no_payload() noexcept
      {
        ROCKET_ASSERT(this->m_hresp >= hresp_headers_done);
        this->m_parser->flags |= F_SKIPBODY;
      }

    // Get the parsed headers.
    bool
    should_close_after_payload() const noexcept
      { return this->m_close_after_payload;  }

    bool
    headers_complete() const noexcept
      { return this->m_hresp >= hresp_headers_done;  }

    const HTTP_Response_Headers&
    headers() const noexcept
      { return this->m_headers;  }

    HTTP_Response_Headers&
    mut_headers() noexcept
      { return this->m_headers;  }

    // Parses the payload of an HTTP response from a stream. `data` may be consumed
    // partially, and must be preserved between calls. If `payload_complete()`
    // returns `true` before the call, this function does nothing.
    void
    parse_payload_from_stream(linear_buffer& data, bool eof);

    // Get the parsed payload.
    bool
    payload_complete() const noexcept
      { return this->m_hresp >= hresp_payload_done;  }

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
        ROCKET_ASSERT(this->m_hresp >= hresp_payload_done);

        this->m_headers.clear();
        this->m_payload.clear();
        this->m_hresp = hresp_new;
        this->m_close_after_payload = false;
      }
  };

}  // namespace poseidon
#endif
