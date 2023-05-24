// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FIBER_
#define POSEIDON_FIBER_ABSTRACT_FIBER_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Fiber
  {
  private:
    friend class Fiber_Scheduler;

    callback_thunk_ptr<const Abstract_Fiber*, shptrR<Abstract_Future>, milliseconds> m_yield = nullptr;
    Fiber_Scheduler* m_sched = nullptr;

    atomic_relaxed<Async_State> m_state;

  protected:
    // Constructs an empty fiber.
    explicit
    Abstract_Fiber() noexcept;

  protected:
    // Gets the scheduler instance inside one of the fiber callbacks.
    // If this function is called elsewhere, the behavior is undefined.
    Fiber_Scheduler&
    do_abstract_fiber_scheduler() const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_sched, "this function shall only be called inside a fiber");
        return *(this->m_sched);
      }

    // This callback is invoked by the fiber scheduler and is intended to be
    // overriden by derived classes to perform useful operation.
    virtual
    void
    do_abstract_fiber_on_work() = 0;

    // Yields execution to another fiber. If `futr_opt` is non-null, the
    // current fiber is suspended until `*futr_opt` becomes ready.
    void
    do_yield(shptrR<Abstract_Future> futr_opt, milliseconds fail_timeout_override = zero_duration) const;

    // This callback is invoked before `do_abstract_fiber_on_execution()`, and
    // after it is resumed from a preivous yield operation. `async_state()` can
    // be used to examine the current operation.
    // The default implementations merely print a message.
    virtual
    void
    do_abstract_fiber_on_resumed() noexcept;

    // This callback is invoked after `do_abstract_fiber_on_execution()`, and
    // before it is suspended by a yield operation. `async_state()` can  be
    // used to examine the current operation.
    // The default implementations merely print a message.
    virtual
    void
    do_abstract_fiber_on_suspended() noexcept;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Abstract_Fiber);

    // Gets the schedule state.
    Async_State
    async_state() const noexcept
      { return this->m_state.load();  }
  };

}  // namespace poseidon
#endif
