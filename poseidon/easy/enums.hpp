// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_ENUMS_
#define POSEIDON_EASY_ENUMS_

#include "../fwd.hpp"
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
    easy_ws_pong     = 34,  // pong notification received
    easy_ws_close    = 35,  // closure notification received
  };

enum Easy_HWS_Event : uint8_t
  {
    easy_hws_open     = 41,  // connection established
    easy_hws_text     = 42,  // text message received
    easy_hws_binary   = 43,  // binary message received
    easy_hws_pong     = 44,  // pong notification received
    easy_hws_close    = 45,  // closure notification received
    easy_hws_get      = 46,  // simple HTTP GET
    easy_hws_head     = 47,  // simple HTTP HEAD
  };

}  // namespace poseidon
#endif
