// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "websocket_frame_header.hpp"
#include "../utils.hpp"
namespace poseidon {

void
WebSocket_Frame_Header::
encode(tinyfmt& fmt)
  const
  {
    // Write the opcode byte. Fields are written according to the figure in
    // 5.2. Base Framing Protocol, RFC 6455, in big-endian byte order.
    char bytes[16];
    uint32_t ntotal = 2;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
    // Set the opcode byte.
    bytes[0] = this->fin << 7 | this->rsv1 << 6 | this->rsv2 << 5 | this->rsv3 << 4
               | this->opcode;

    if(this->payload_len <= 125) {
      // one-byte length
      bytes[1] = this->masked << 7 | this->payload_len;
    }
    else if(this->payload_len <= 65535) {
      // two-byte length
      bytes[1] = this->masked << 7 | 126;
      ntotal += 2;
      ::rocket::store_be<uint16_t>(bytes + ntotal - 2, this->payload_len);
    }
    else {
      // eight-byte length
      bytes[1] = this->masked << 7 | 127;
      ntotal += 8;
      ::rocket::store_be<uint64_t>(bytes + ntotal - 8, this->payload_len);
    }

    if(this->masked) {
      // four-byte masking key
      ntotal += 4;
      ::rocket::store_be<uint32_t>(bytes + ntotal - 4, this->masking_key);
    }
#pragma GCC diagnostic pop

    fmt.putn(bytes, ntotal);
  }

void
WebSocket_Frame_Header::
mask_payload(char* data, size_t size)
  noexcept
  {
    if(!this->masked || (this->masking_key == 0))
      return;

    char* cur = data;
    char* esdata = data + size;
    uint32_t& key = this->masking_key;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
    // Align the pointer.
    while(reinterpret_cast<uintptr_t>(cur) % 4 != 0)
      if(cur == esdata)
        return;
      else {
        key = key << 8 | key >> 24;
        *cur ^= key;
        cur ++;
      }

    if(esdata - cur >= 4) {
      // Do it in the SIMD way. This branch must not alter `key`.
      uint32_t bekey = ::rocket::load_be<uint32_t>(&key);
      __m256 ymask = _mm256_broadcast_ss(reinterpret_cast<float*>(&bekey));

      while(esdata - cur >= 32) {
        float* ycur = reinterpret_cast<float*>(cur);
        _mm256_storeu_ps(ycur, _mm256_xor_ps(_mm256_loadu_ps(ycur), ymask));
        cur += 32;
      }

      while(esdata - cur >= 4) {
        uint32_t* wcur = reinterpret_cast<uint32_t*>(cur);
        *wcur ^= bekey;
        cur += 4;
      }
    }

    while(esdata - cur >= 1) {
      key = key << 8 | key >> 24;
      *cur ^= key;
      cur ++;
    }
#pragma GCC diagnostic pop

    ROCKET_ASSERT(cur == esdata);
  }

}  // namespace poseidon
