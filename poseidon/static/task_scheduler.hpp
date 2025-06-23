// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_TASK_SCHEDULER_
#define POSEIDON_STATIC_TASK_SCHEDULER_

#include "../fwd.hpp"
namespace poseidon {

class Task_Scheduler
  {
  private:
    mutable plain_mutex m_queue_mutex;
    condition_variable m_queue_avail;
    cow_vector<wkptr<Abstract_Task>> m_queue_front;
    cow_vector<wkptr<Abstract_Task>> m_queue_back;

  public:
    // Creates an empty task scheduler.
    Task_Scheduler() noexcept;

  public:
    Task_Scheduler(const Task_Scheduler&) = delete;
    Task_Scheduler& operator=(const Task_Scheduler&) & = delete;
    ~Task_Scheduler();

    // Pops and executes a task.
    // This function should be called by the task thread repeatedly.
    void
    thread_loop();

    // Enqueues a task.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    enqueue(const shptr<Abstract_Task>& task);
  };

}  // namespace poseidon
#endif
