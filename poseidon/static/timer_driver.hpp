// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_TIMER_DRIVER_
#define POSEIDON_STATIC_TIMER_DRIVER_

#include "../fwd.hpp"
namespace poseidon {

class Timer_Driver
  {
  private:
    mutable plain_mutex m_pq_mutex;
    condition_variable m_pq_avail;
    struct X_Queued_Timer;
    cow_vector<X_Queued_Timer> m_pq;

  public:
    // Constructs an empty driver.
    Timer_Driver() noexcept;

  public:
    Timer_Driver(const Timer_Driver&) = delete;
    Timer_Driver& operator=(const Timer_Driver&) & = delete;
    ~Timer_Driver();

    // Schedules timers.
    // This function should be called by the timer thread repeatedly.
    void
    thread_loop();

    // Inserts a timer. `delay` specifies the number of milliseconds that a timer
    // will be triggered after it is inserted successfully. `period` is the number
    // of milliseconds of intervals for periodic timers. `period` can be zero to
    // denote a one-shot timer.
    // This function is thread-safe.
    void
    insert(const shptr<Abstract_Timer>& timer, milliseconds delay, milliseconds period);
  };

}  // namespace poseidon
#endif
