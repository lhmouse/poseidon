// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "websocket_frame_header.hpp"
#include "../utils.hpp"
namespace poseidon {

void
WebSocket_Frame_Header::
encode(tinyfmt& fmt) const
  {
    // Write the opcode byte. Fields are written according to the figure in
    // 5.2. Base Framing Protocol, RFC 6455, in big-endian byte order.
    int ch = this->fin << 7 | this->rsv1 << 6 | this->rsv2 << 5 | this->rsv3 << 4 | this->opcode;
    fmt.putc((char) ch);

    if(this->payload_len <= 125) {
      // one-byte length
      ch = this->mask << 7 | (int) this->payload_len;
      fmt.putc((char) ch);
    }
    else if(this->payload_len <= 65535) {
      // two-byte length
      ch = this->mask << 7 | 126;
      fmt.putc((char) ch);
      uint16_t belen = ROCKET_HTOBE16((uint16_t) this->payload_len);
      fmt.putn((const char*) &belen, 2);
    }
    else {
      // eight-byte length
      ch = this->mask << 7 | 127;
      fmt.putc((char) ch);
      uint64_t belen = ROCKET_HTOBE64(this->payload_len);
      fmt.putn((const char*) &belen, 8);
    }

    if(this->mask)
      fmt.putn((const char*) &(this->mask_key), 4);
  }

void
WebSocket_Frame_Header::
mask_payload(char* data, size_t size) noexcept
  {
    if(!this->mask)
      return;

    char* cur = data;
    char* esdata = data + size;
    uint32_t mask = ROCKET_LETOH32(mask_key);
    __m128i xmm_mask = _mm_set1_epi32((int) mask);

    while(esdata - cur >= 16) {
      // XMMWORD-wise
      __m128i* xcur = (__m128i*) cur;
      _mm_storeu_si128(xcur, _mm_xor_si128(xmm_mask, _mm_loadu_si128(xcur)));
      cur += 16;
    }

    while(cur != esdata) {
      // bytewise
      *cur = (char) (mask ^ (uint8_t) *cur);
      cur ++;
      mask = mask << 24 | mask >> 8;
    }
  }

}  // namespace poseidon
