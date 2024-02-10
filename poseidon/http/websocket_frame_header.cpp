// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

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

size_t
WebSocket_Frame_Header::
mask_payload(char* data, size_t size) noexcept
  {
    if(!this->mask)
      return 0;

    mask_string(data, size, &(this->mask_key), this->mask_key);
    return size;
  }

}  // namespace poseidon
