// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_fiber.hpp"
#include "../utils.hpp"
namespace poseidon {

Abstract_Fiber::
Abstract_Fiber()
  noexcept
  {
    this->m_scheduler = reinterpret_cast<Fiber_Scheduler*>(-1);
    this->m_sched_yield_fn = reinterpret_cast<sched_yield_fn*>(-3);
  }

Abstract_Fiber::
~Abstract_Fiber()
  {
  }

void
Abstract_Fiber::
yield(const shptr<Abstract_Future>& futr_opt)
  const
  {
    (* this->m_sched_yield_fn) (futr_opt);
  }

}  // namespace poseidon
