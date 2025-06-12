// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_
#define POSEIDON_HTTP_WEBSOCKET_FRAME_HEADER_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

struct WebSocket_Frame_Header
  {
    union {
      __m128i packed_fields_1 = { };
      struct {
        // The payload length is always stored as an `uint64_t`. The `encode()`
        // function will deduce the correct field basing on its value.
        // Reference: https://datatracker.ietf.org/doc/html/rfc6455
        WebSocket_Opcode opcode : 4;
        uint8_t rsv3 : 1;
        uint8_t rsv2 : 1;
        uint8_t rsv1 : 1;
        uint8_t fin : 1;
        uint8_t reserved_1 : 7;
        uint8_t masked : 1;
        uint16_t reserved_2;
        uint32_t masking_key;
        uint64_t payload_len;
      };
    };

    WebSocket_Frame_Header&
    swap(WebSocket_Frame_Header& other) noexcept
      {
        ::std::swap(this->packed_fields_1, other.packed_fields_1);
        return *this;
      }

    // Clears all fields.
    void
    clear() noexcept
      {
        this->packed_fields_1 = _mm_setzero_si128();
      }

    // Encodes this frame header in wire format. The output will be suitable
    // for sending through a stream socket.
    void
    encode(tinyfmt& fmt) const;

    // Masks a part (or unmasks a masked part) of the frame payload, and update
    // `masking_key` incrementally. If `masked` is unset, this function does nothing.
    void
    mask_payload(char* data, size_t size) noexcept;
  };

inline
void
swap(WebSocket_Frame_Header& lhs, WebSocket_Frame_Header& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
