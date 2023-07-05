// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_
#define POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_

#include "../fwd.hpp"
namespace poseidon {

struct WebSocket_Frame_Header
  {
    union {
      struct {
        // These fields resemble the WebSocket frame header. The payload length is
        // always stored as a 64-bit integer. `compressed` corresponds to the RSV1
        // bit.
        // Reference: https://datatracker.ietf.org/doc/html/rfc6455
        // Reference: https://datatracker.ietf.org/doc/html/rfc7692
        bool fin = false;
        bool compressed = false;
        uint8_t m_unused = 0;
        uint8_t opcode = 0;
        char masking_key[4] = { };
        uint64_t payload_length = 0;
      };
      __m128i m_stor;
    };

    // Define some helper functions.
    constexpr
    WebSocket_Frame_Header() noexcept = default;

    WebSocket_Frame_Header&
    swap(WebSocket_Frame_Header& other) noexcept
      {
        ::std::swap(this->m_stor, other.m_stor);
        return *this;
      }
  };

inline
void
swap(WebSocket_Frame_Header& lhs, WebSocket_Frame_Header& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace poseidon
#endif
