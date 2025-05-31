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
    char bytes[16];
    uint32_t len = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
    bytes[len++] = this->fin << 7 | this->rsv1 << 6 | this->rsv2 << 5
                   | this->rsv3 << 4 | this->opcode;

    if(this->payload_len <= 125) {
      // one-byte length
      bytes[len++] = this->mask << 7 | this->payload_len;
    }
    else if(this->payload_len <= 65535) {
      // two-byte length
      bytes[len++] = this->mask << 7 | 126;
      bytes[len++] = this->payload_len >> 8;
      bytes[len++] = this->payload_len;
    }
    else {
      // eight-byte length
      bytes[len++] = this->mask << 7 | 127;
      bytes[len++] = this->payload_len >> 56;
      bytes[len++] = this->payload_len >> 48;
      bytes[len++] = this->payload_len >> 40;
      bytes[len++] = this->payload_len >> 32;
      bytes[len++] = this->payload_len >> 24;
      bytes[len++] = this->payload_len >> 16;
      bytes[len++] = this->payload_len >> 8;
      bytes[len++] = this->payload_len;
    }

    if(this->mask) {
      bytes[len++] = this->mask_key;
      bytes[len++] = this->mask_key >> 8;
      bytes[len++] = this->mask_key >> 16;
      bytes[len++] = this->mask_key >> 24;
    }
#pragma GCC diagnostic pop

    fmt.putn(bytes, len);
  }

void
WebSocket_Frame_Header::
mask_payload(char* data, size_t size) noexcept
  {
    if(!this->mask)
      return;

    char* cur = data;
    char* esdata = data + size;
    uint32_t dw_mask = this->mask_key;
    __m128i xmm_mask = _mm_set1_epi32(static_cast<int>(this->mask_key));

    while(esdata - cur >= 16) {
      __m128i* xmmp = reinterpret_cast<__m128i*>(cur);
      _mm_storeu_si128(xmmp, _mm_xor_si128(xmm_mask, _mm_loadu_si128(xmmp)));
      cur += 16;
    }

    while(esdata - cur >= 4) {
      uint32_t* dp = reinterpret_cast<uint32_t*>(cur);
      *dp = *dp ^ dw_mask;
      cur += 4;
    }

    while(esdata != cur) {
      *cur = static_cast<char>(static_cast<uint8_t>(*cur) ^ dw_mask);
      dw_mask = dw_mask << 24 | dw_mask >> 8;
      cur ++;
    }
  }

}  // namespace poseidon
