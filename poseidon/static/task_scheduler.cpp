// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "task_scheduler.hpp"
#include "../base/abstract_task.hpp"
#include "../utils.hpp"
namespace poseidon {

Task_Scheduler::
Task_Scheduler() noexcept
  {
  }

Task_Scheduler::
~Task_Scheduler()
  {
  }

void
Task_Scheduler::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    while(this->m_queue_front.empty() && this->m_queue_back.empty())
      this->m_queue_avail.wait(lock);

    if(this->m_queue_front.empty())
      this->m_queue_front.swap(this->m_queue_back);

    auto task = move(this->m_queue_front.back());
    this->m_queue_front.pop_back();
    if(!task || task->m_abandoned.load())
      return;
    recursive_mutex::unique_lock sched_lock(task->m_sched_mutex);
    task->m_scheduler = this;
    lock.unlock();

    // Execute it. Exceptions are ignored.
    POSEIDON_LOG_TRACE(("Executing task `$1` (class `$2`)"), task, typeid(*task));
    POSEIDON_CATCH_EVERYTHING(task->do_on_abstract_task_execute());
    task->m_scheduler = reinterpret_cast<Task_Scheduler*>(-5);
  }

void
Task_Scheduler::
launch(const shptr<Abstract_Task>& task)
  {
    if(!task)
      POSEIDON_THROW(("Null task pointer not valid"));

    // Insert the task.
    plain_mutex::unique_lock lock(this->m_queue_mutex);
    this->m_queue_back.emplace_back(task);
    this->m_queue_avail.notify_one();
  }

}  // namespace poseidon
