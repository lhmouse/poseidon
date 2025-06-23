// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_task.hpp"
namespace poseidon {

Abstract_Task::
Abstract_Task() noexcept
  {
    this->m_executor = reinterpret_cast<Task_Executor*>(-1);
  }

Abstract_Task::
~Abstract_Task()
  {
  }

Task_Executor&
Abstract_Task::
do_abstract_task_lock_executor(recursive_mutex::unique_lock& lock) const noexcept
  {
    lock.lock(this->m_exec_mutex);
    ROCKET_ASSERT(this->m_executor);
    return *(this->m_executor);
  }

}  // namespace poseidon
