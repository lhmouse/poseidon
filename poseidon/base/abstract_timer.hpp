// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ABSTRACT_TIMER_
#define POSEIDON_BASE_ABSTRACT_TIMER_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Timer
  {
  private:
    friend class Timer_Driver;

  protected:
    // Constructs an inactive timer.
    Abstract_Timer() noexcept;

  protected:
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
