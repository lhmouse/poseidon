// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "task_executor.hpp"
#include "../base/abstract_task.hpp"
#include "../utils.hpp"
namespace poseidon {

Task_Executor::
Task_Executor() noexcept
  {
  }

Task_Executor::
~Task_Executor()
  {
  }

void
Task_Executor::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    while(this->m_queue_front.empty() && this->m_queue_back.empty())
      this->m_queue_avail.wait(lock);

    if(this->m_queue_front.empty())
      this->m_queue_front.swap(this->m_queue_back);

    auto task = this->m_queue_front.back().lock();
    this->m_queue_front.pop_back();
    lock.unlock();

    if(!task)
      return;

    // Execute it. Exceptions are ignored.
    POSEIDON_LOG_TRACE(("Executing task `$1` (class `$2`)"), task, typeid(*task));
    POSEIDON_CATCH_EVERYTHING(task->do_on_abstract_task_execute());
    POSEIDON_CATCH_EVERYTHING(task->do_on_abstract_task_finalize());
  }

void
Task_Executor::
enqueue(const shptr<Abstract_Task>& task)
  {
    if(!task)
      POSEIDON_THROW(("Null task pointer not valid"));

    // Insert the task.
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    this->m_queue_back.emplace_back(task);
    this->m_queue_avail.notify_one();
  }

}  // namespace poseidon
