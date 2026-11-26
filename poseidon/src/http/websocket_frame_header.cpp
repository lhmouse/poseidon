// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/websocket_frame_header.hpp"
#include "../../utils.hpp"
namespace poseidon {

void
WebSocket_Frame_Header::
encode(tinyfmt& fmt) const
  {
    // Write the opcode byte. Fields are written according to the figure in
    // 5.2. Base Framing Protocol, RFC 6455, in big-endian byte order.
    char bytes[16];
    uint32_t ntotal = 2;

    // Set the opcode byte.
    bytes[0] = static_cast<char>(
        this->fin << 7 | this->rsv1 << 6 | this->rsv2 << 5 | this->rsv3 << 4 | this->opcode);

    if(this->payload_len <= 125) {
      // one-byte length
      bytes[1] = static_cast<char>(this->masked << 7 | this->payload_len);
    }
    else if(this->payload_len <= 65535) {
      // two-byte length
      bytes[1] = static_cast<char>(this->masked << 7 | 126);
      ::asteria::store_be<uint16_t>(bytes + ntotal, static_cast<uint16_t>(this->payload_len));
      ntotal += 2;
    }
    else {
      // eight-byte length
      bytes[1] = static_cast<char>(this->masked << 7 | 127);
      ::asteria::store_be<uint64_t>(bytes + ntotal, this->payload_len);
      ntotal += 8;
    }

    if(this->masked) {
      // four-byte masking key
      ::memcpy(bytes + ntotal, &(this->masking_key), 4);
      ntotal += 4;
    }

    fmt.putn(bytes, ntotal);
  }

void
WebSocket_Frame_Header::
mask_payload(char* data, size_t size) noexcept
  {
    if(!this->masked || (this->masking_key == 0))
      return;

    char* cur = data;
    char* esdata = data + size;
    uint32_t reg_key = this->masking_key;

    // Align to four bytes.
    while(reinterpret_cast<uintptr_t>(cur) % 4 != 0) {
      if(cur == esdata)
        return;

      *cur ^= static_cast<char>(reg_key);
      this->masking_key = reg_key = __rord(reg_key, 8);
      cur ++;
    }

#if defined __AVX2__
    if(esdata - cur >= 128) {
      __m256i ymm_mask = _mm256_set1_epi32(static_cast<int>(reg_key));

      while(reinterpret_cast<uintptr_t>(cur) % 32 != 0) {
        if(cur == esdata)
          return;

        uint32_t* wcur = reinterpret_cast<uint32_t*>(cur);
        *wcur ^= reg_key;
        cur += 4;
      }

      while(esdata - cur >= 32) {
        __m256i* ycur = reinterpret_cast<__m256i*>(cur);
        _mm256_store_si256(ycur, _mm256_xor_si256(_mm256_load_si256(ycur), ymm_mask));
        cur += 32;
      }
    }
#endif

    while(esdata - cur >= 4) {
      uint32_t* wcur = reinterpret_cast<uint32_t*>(cur);
      *wcur ^= reg_key;
      cur += 4;
    }

    while(esdata - cur >= 1) {
      *cur ^= static_cast<char>(reg_key);
      this->masking_key = reg_key = __rord(reg_key, 8);
      cur ++;
    }

    ASTERIA_ASSERT(cur == esdata);
  }

}  // namespace poseidon
