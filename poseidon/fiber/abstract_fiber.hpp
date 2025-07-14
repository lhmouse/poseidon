// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FIBER_
#define POSEIDON_FIBER_ABSTRACT_FIBER_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Fiber
  {
  private:
    friend class Fiber_Scheduler;

    atomic_relaxed<bool> m_abandoned;
    mutable recursive_mutex m_sched_mutex;
    Fiber_Scheduler* m_scheduler;
    using sched_yield_fn = void (const shptr<Abstract_Future>&);
    sched_yield_fn* m_sched_yield_fn;

  protected:
    // Constructs an inactive fiber.
    Abstract_Fiber()
      noexcept;

  protected:
    // Get the fiber scheduler instance inside the callbacks hereafter. If
    // this function is called elsewhere, the behavior is undefined.
    Fiber_Scheduler&
    do_abstract_fiber_lock_scheduler(recursive_mutex::unique_lock& lock)
      const noexcept;

    // This callback is invoked by the fiber scheduler and is intended to be
    // overriden by derived classes to perform useful operation.
    virtual
    void
    do_on_abstract_fiber_execute()
      = 0;

  public:
    Abstract_Fiber(const Abstract_Fiber&) = delete;
    Abstract_Fiber& operator=(const Abstract_Fiber&) & = delete;
    virtual ~Abstract_Fiber();

    // Mark this fiber as abandoned to prevent it from being scheduled. This
    // operation cannot be undone.
    void
    abandon()
      noexcept
      { this->m_abandoned.store(true);  }

    // Suspends execution of the current fiber. If `futr_opt` is not null, it
    // is suspended until `*futr_opt` becomes ready.
    void
    yield(const shptr<Abstract_Future>& futr_opt)
      const;
  };

}  // namespace poseidon
#endif
