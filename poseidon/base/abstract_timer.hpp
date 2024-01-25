// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ABSTRACT_TIMER_
#define POSEIDON_BASE_ABSTRACT_TIMER_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Timer
  {
  private:
    friend class Timer_Driver;

    atomic_relaxed<Async_State> m_state;

  protected:
    // Constructs an inactive timer.
    Abstract_Timer();

  protected:
    // This callback is invoked by the timer thread and is intended to be
    // overriden by derived classes.
    virtual
    void
    do_abstract_timer_on_tick(steady_time now) = 0;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Abstract_Timer);

    // Gets the schedule state.
    Async_State
    async_state() const noexcept
      { return this->m_state.load();  }
  };

}  // namespace poseidon
#endif
