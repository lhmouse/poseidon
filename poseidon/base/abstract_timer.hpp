// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ABSTRACT_TIMER_
#define POSEIDON_BASE_ABSTRACT_TIMER_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Timer
  {
  private:
    friend class Timer_Scheduler;

    mutable recursive_mutex m_sched_mutex;
    Timer_Scheduler* m_scheduler;

  protected:
    // Constructs an inactive timer.
    Abstract_Timer() noexcept;

  protected:
    // Get the timer scheduler instance inside the callbacks hereafter. If this
    // function is called elsewhere, the behavior is undefined.
    Timer_Scheduler&
    do_abstract_timer_lock_scheduler(recursive_mutex::unique_lock& lock) const noexcept;

    // This callback is invoked by the timer thread and is intended to be
    // overriden by derived classes.
    virtual
    void
    do_abstract_timer_on_tick(steady_time now) = 0;

  public:
    virtual ~Abstract_Timer();
  };

}  // namespace poseidon
#endif
