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

    atomic_relaxed<Async_State> m_state;

    using yield_function = void (Fiber_Scheduler*, shptrR<Abstract_Future>, milliseconds);
    yield_function* m_yield = nullptr;
    Fiber_Scheduler* m_sched = nullptr;

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
        auto sched = this->m_sched;
        ROCKET_ASSERT_MSG(sched, "this function shall only be called inside a fiber");
        return *sched;
      }

    // This callback is invoked by the fiber scheduler and is intended to be
    // overriden by derived classes to perform useful operation.
    virtual
    void
    do_abstract_fiber_on_work() = 0;

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

    // Yields execution to another fiber. If `futr_opt` is not null, the
    // current fiber is suspended until `*futr_opt` becomes ready. If
    // `fail_timeout_override` is not zero, it specifies the maximum timeout
    // that the current fiber can be suspended.
    void
    yield(shptrR<Abstract_Future> futr_opt, milliseconds fail_timeout_override = zero_duration) const;
  };

}  // namespace poseidon
#endif
