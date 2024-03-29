// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ABSTRACT_ASYNC_TASK_
#define POSEIDON_BASE_ABSTRACT_ASYNC_TASK_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Async_Task
  {
  private:
    friend class Async_Task_Executor;

  protected:
    // Constructs an asynchronous task.
    Abstract_Async_Task() noexcept;

  protected:
    // This callback is invoked by the task executor thread and is intended to
    // be overriden by derived classes.
    virtual
    void
    do_on_abstract_async_task_execute() = 0;

  public:
    virtual ~Abstract_Async_Task();
  };

}  // namespace poseidon
#endif
