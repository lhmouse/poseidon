// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ENUMS_
#define POSEIDON_BASE_ENUMS_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

enum zlib_Format : uint8_t
  {
    zlib_deflate  = 0,  // deflate data with zlib header
    zlib_raw      = 1,  // raw deflate data
    zlib_gzip     = 2,  // deflate data with gzip header
  };

enum Async_State : uint8_t
  {
    async_pending    = 0,
    async_suspended  = 1,
    async_running    = 2,
    async_finished   = 3,
  };

}  // namespace poseidon
#endif
