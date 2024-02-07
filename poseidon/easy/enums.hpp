// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_ENUMS_
#define POSEIDON_EASY_ENUMS_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

enum Easy_Stream_Event : uint8_t
  {
    easy_stream_open     = 11,  // connection established
    easy_stream_data     = 12,  // data received
    easy_stream_close    = 13,  // connection closed
  };

enum Easy_HTTP_Event : uint8_t
  {
    easy_http_open     = 21,  // connection established
    easy_http_message  = 22,  // message received
    easy_http_close    = 23,  // connection closed
  };

enum Easy_WS_Event : uint8_t
  {
    easy_ws_open     = 31,  // connection established
    easy_ws_text     = 32,  // text message received
    easy_ws_binary   = 33,  // binary message received
    easy_ws_ping     = easy_ws_open,
    easy_ws_pong     = 34,  // pong notification received
    easy_ws_close    = 35,  // closure notification closed
  };

}  // namespace poseidon
#endif
