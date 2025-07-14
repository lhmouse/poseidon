// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ABSTRACT_TASK_
#define POSEIDON_BASE_ABSTRACT_TASK_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Task
  {
  private:
    friend class Task_Scheduler;

    atomic_relaxed<bool> m_abandoned;
    mutable recursive_mutex m_sched_mutex;
    Task_Scheduler* m_scheduler;

  protected:
    // Constructs an asynchronous task.
    Abstract_Task()
      noexcept;

  protected:
    // Get the task scheduler instance inside the callbacks hereafter. If this
    // function is called elsewhere, the behavior is undefined.
    Task_Scheduler&
    do_abstract_task_lock_scheduler(recursive_mutex::unique_lock& lock)
      const noexcept;

    // This callback is invoked by the task scheduler thread and is intended to
    // be overriden by derived classes.
    virtual
    void
    do_on_abstract_task_execute()
      = 0;

  public:
    virtual ~Abstract_Task();

    // Mark this task as abandoned to prevent it from being scheduled. This
    // operation cannot be undone.
    void
    abandon()
      noexcept
      { this->m_abandoned.store(true);  }
  };

}  // namespace poseidon
#endif
