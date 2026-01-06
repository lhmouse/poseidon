// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_task.hpp"
namespace poseidon {

Abstract_Task::
Abstract_Task()
  noexcept
  {
    this->m_scheduler = reinterpret_cast<Task_Scheduler*>(-1);
  }

Abstract_Task::
~Abstract_Task()
  {
  }

Task_Scheduler&
Abstract_Task::
do_abstract_task_lock_scheduler(recursive_mutex::unique_lock& lock)
  const noexcept
  {
    lock.lock(this->m_sched_mutex);
    ROCKET_ASSERT(this->m_scheduler);
    return *(this->m_scheduler);
  }

}  // namespace poseidon
