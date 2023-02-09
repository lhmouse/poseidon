// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ENUMS_
#define POSEIDON_BASE_ENUMS_

#include "../fwd.hpp"

namespace poseidon {

enum Async_State : uint8_t
  {
    async_state_pending    = 0,
    async_state_suspended  = 1,
    async_state_running    = 2,
    async_state_finished   = 3,
  };

}  // namespace poseidon
#endif
