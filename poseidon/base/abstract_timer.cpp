// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_timer.hpp"
namespace poseidon {

Abstract_Timer::
Abstract_Timer() noexcept
  {
    this->m_driver = reinterpret_cast<Timer_Driver*>(-1);
  }

Abstract_Timer::
~Abstract_Timer()
  {
  }

Timer_Driver&
Abstract_Timer::
do_abstract_timer_lock_driver(recursive_mutex::unique_lock& lock) const noexcept
  {
    lock.lock(this->m_driver_mutex);
    ROCKET_ASSERT(this->m_driver);
    return *(this->m_driver);
  }

}  // namespace poseidon
