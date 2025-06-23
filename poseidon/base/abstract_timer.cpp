// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_timer.hpp"
namespace poseidon {

Abstract_Timer::
Abstract_Timer() noexcept
  {
    this->m_scheduler = reinterpret_cast<Timer_Scheduler*>(-1);
  }

Abstract_Timer::
~Abstract_Timer()
  {
  }

Timer_Scheduler&
Abstract_Timer::
do_abstract_timer_lock_scheduler(recursive_mutex::unique_lock& lock) const noexcept
  {
    lock.lock(this->m_sched_mutex);
    ROCKET_ASSERT(this->m_scheduler);
    return *(this->m_scheduler);
  }

}  // namespace poseidon
