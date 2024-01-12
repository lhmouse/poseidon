// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_
#define POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_

#include "../fwd.hpp"
namespace poseidon {

struct WebSocket_Frame_Header
  {
    union {
      __m128i m_stor;

      struct {
        // The payload length is always stored as an `uint64_t`. The `encode()`
        // function will deduce the correct field basing on its value.
        // Reference: https://datatracker.ietf.org/doc/html/rfc6455
        uint8_t opcode : 4;
        uint8_t rsv3 : 1;
        uint8_t rsv2 : 1;
        uint8_t rsv1 : 1;
        uint8_t fin : 1;
        uint8_t reserved_1 : 7;
        uint8_t mask : 1;
        uint16_t reserved_2;
        union {
          char mask_key[4];
          uint32_t mask_key_u32;
          int32_t mask_key_i32;
        };
        uint64_t payload_len;
      };
    };

    // Define some helper functions.
    constexpr
    WebSocket_Frame_Header() noexcept
      :
        m_stor()
      { }

    WebSocket_Frame_Header&
    swap(WebSocket_Frame_Header& other) noexcept
      {
        ::std::swap(this->m_stor, other.m_stor);
        return *this;
      }

    // Clears all fields.
    void
    clear() noexcept
      {
        this->m_stor = _mm_setzero_si128();
      }

    // Encodes this frame header in wire format. The output will be suitable
    // for sending through a stream socket.
    void
    encode(tinyfmt& fmt) const;

    // Masks a part (or unmasks a masked part) of the frame payload, and update
    // `mask_key` incrementally. If `mask` is unset, this function does nothing.
    size_t
    mask_payload(char* data, size_t size) noexcept;
  };

inline
void
swap(WebSocket_Frame_Header& lhs, WebSocket_Frame_Header& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
