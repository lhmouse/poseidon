// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_REDIS_FWD_
#define POSEIDON_THIRD_REDIS_FWD_

#include "../fwd.hpp"
#include "hiredis/hiredis.h"
namespace poseidon {

struct redisContext_deleter
  {
    void
    operator()(::redisContext* p) const noexcept
      { ::redisFree(p);  }
  };

struct redisReply_deleter
  {
    void
    operator()(::redisReply* p) const noexcept
      { ::freeReplyObject(p);  }
  };

using uniptr_redisContext = ::rocket::unique_ptr<::redisContext, redisContext_deleter>;
using uniptr_redisReply = ::rocket::unique_ptr<::redisReply, redisReply_deleter>;

}  // namespace poseidon
#endif
