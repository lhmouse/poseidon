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
    uint32_t ntotal = 2;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
    bytes[0] = this->fin << 7 | this->rsv1 << 6 | this->rsv2 << 5 | this->rsv3 << 4
               | this->opcode;

    if(this->payload_len <= 125) {
      // one-byte length
      bytes[1] = this->mask << 7 | this->payload_len;
    }
    else if(this->payload_len <= 65535) {
      // two-byte length
      bytes[1] = this->mask << 7 | 126;
      ntotal += 2;
      uint16_t belen = ROCKET_HTOBE16(this->payload_len);
      ::memcpy(bytes + ntotal - 2, &belen, 2);
    }
    else {
      // eight-byte length
      bytes[1] = this->mask << 7 | 127;
      ntotal += 8;
      uint64_t belen = ROCKET_HTOBE64(this->payload_len);
      ::memcpy(bytes + ntotal - 8, &belen, 8);
    }

    if(this->mask) {
      // four-byte masking key
      ntotal += 4;
      uint32_t lekey = ROCKET_HTOLE32(this->mask_key);
      ::memcpy(bytes + ntotal - 4, &lekey, 4);
    }
#pragma GCC diagnostic pop

    fmt.putn(bytes, ntotal);
  }

void
WebSocket_Frame_Header::
mask_payload(char* data, size_t size) noexcept
  {
    if(!this->mask)
      return;

    char* cur = data;
    char* esdata = data + size;
    uint32_t dw_key = this->mask_key;
    __m128i xmm_key = _mm_set1_epi32(static_cast<int>(dw_key));

    while(esdata - cur >= 16) {
      __m128i* xmmp = reinterpret_cast<__m128i*>(cur);
      _mm_storeu_si128(xmmp, _mm_xor_si128(xmm_key, _mm_loadu_si128(xmmp)));
      cur += 16;
    }

    while(esdata - cur >= 4) {
      uint32_t* dp = reinterpret_cast<uint32_t*>(cur);
      *dp = *dp ^ dw_key;
      cur += 4;
    }

    while(esdata != cur) {
      *cur = static_cast<char>(static_cast<uint8_t>(*cur) ^ dw_key);
      dw_key = dw_key << 24 | dw_key >> 8;
      cur ++;
    }
  }

}  // namespace poseidon
