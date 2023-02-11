// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_ENUMS_
#define POSEIDON_EASY_ENUMS_

#include "../fwd.hpp"
namespace poseidon {

enum Connection_Event : uint8_t
  {
    connection_event_null    = 0,
    connection_event_open    = 1,
    connection_event_stream  = 2,
    connection_event_closed  = 3,
  };

}  // namespace poseidon
#endif
