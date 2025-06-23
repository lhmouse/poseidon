// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ABSTRACT_TASK_
#define POSEIDON_BASE_ABSTRACT_TASK_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Task
  {
  private:
    friend class Task_Executor;

    mutable recursive_mutex m_exec_mutex;
    Task_Executor* m_executor;

  protected:
    // Constructs an asynchronous task.
    Abstract_Task() noexcept;

  protected:
    // Get the task executor instance inside the callbacks hereafter. If this
    // function is called elsewhere, the behavior is undefined.
    Task_Executor&
    do_abstract_task_lock_executor(recursive_mutex::unique_lock& lock) const noexcept;

    // This callback is invoked by the task executor thread and is intended to
    // be overriden by derived classes.
    virtual
    void
    do_on_abstract_task_execute() = 0;

  public:
    virtual ~Abstract_Task();
  };

}  // namespace poseidon
#endif
