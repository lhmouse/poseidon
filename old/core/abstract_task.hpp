// This file is part of Poseidon.
// Copyleft 2020, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_CORE_ABSTRACT_TASK_
#define POSEIDON_CORE_ABSTRACT_TASK_

#include "../fwd.hpp"

namespace poseidon {

class Abstract_Task
  {
    friend Task_Executor_Pool;

  private:
    uintptr_t m_key;
    atomic_relaxed<bool> m_cancelled;
    atomic_relaxed<Async_State> m_state;

  protected:
    explicit
    Abstract_Task(uintptr_t key) noexcept
      : m_key(key)
      { }

    POSEIDON_DELETE_COPY(Abstract_Task);

  private:
    // Executes this task and satisfies some promise of the derived class.
    // This function is called only once. No matter whether it returns or
    // throws an exception, this task is deleted from the executor queue.
    virtual
    void
    do_abstract_task_execute()
      = 0;

  public:
    virtual
    ~Abstract_Task();

    // Marks this task to be deleted.
    bool
    cancel() noexcept
      { return this->m_cancelled.xchg(true);  }

    // Gets the asynchrnous state, which is set by executor threads.
    ROCKET_PURE
    Async_State
    state() const noexcept
      { return this->m_state.load();  }
  };

}  // namespace poseidon

#endif