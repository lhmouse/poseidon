// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_REDIS_ENUMS_
#define POSEIDON_REDIS_ENUMS_

#include "../fwd.hpp"
namespace poseidon {

enum Redis_Value_Type : uint8_t
  {
    redis_value_null      = 0,  // REDIS_REPLY_NIL
    redis_value_integer   = 1,  // REDIS_REPLY_INTEGER
    redis_value_string    = 2,  // REDIS_REPLY_STRING
    redis_value_array     = 3,  // REDIS_REPLY_ARRAY
  };

}  // namespace poseidon
#endif
