// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_fiber.hpp"
#include "../utils.hpp"
namespace poseidon {

Abstract_Fiber::
Abstract_Fiber() noexcept
  {
  }

Abstract_Fiber::
~Abstract_Fiber()
  {
  }

void
Abstract_Fiber::
do_on_abstract_fiber_resumed()
  {
    POSEIDON_LOG_TRACE(("Resumed fiber `$1` (class `$2`)"), this, typeid(*this));
  }

void
Abstract_Fiber::
do_on_abstract_fiber_suspended()
  {
    POSEIDON_LOG_TRACE(("Suspended fiber `$1` (class `$2`)"), this, typeid(*this));
  }

}  // namespace poseidon
