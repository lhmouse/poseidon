// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_
#define POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_

#include "../fwd.hpp"
namespace poseidon {

struct WebSocket_Frame_Header
  {
    union {
      __m128i m_stor = { };

      struct {
        // The payload length is always stored as a 64-bit integer. If the
        // Per-Message Compression Extension (PMCE) is enabled, `compressed`
        // corresponds to the RSV1 bit.
        // Reference: https://datatracker.ietf.org/doc/html/rfc6455
        // Reference: https://datatracker.ietf.org/doc/html/rfc7692
        uint32_t fin : 1;
        uint32_t compressed : 1;
        uint32_t rsv2 : 1;
        uint32_t rsv3 : 1;
        uint32_t opcode : 4;
        uint32_t mask : 1;
        uint32_t reserved_1 : 23;
        uint8_t masking_key[4];
        uint64_t payload_length;
      };
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
