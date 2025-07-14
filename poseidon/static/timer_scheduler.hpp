// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_TIMER_SCHEDULER_
#define POSEIDON_STATIC_TIMER_SCHEDULER_

#include "../fwd.hpp"
namespace poseidon {

class Timer_Scheduler
  {
  private:
    mutable plain_mutex m_pq_mutex;
    condition_variable m_pq_avail;
    struct X_Queued_Timer;
    cow_vector<X_Queued_Timer> m_pq;

  public:
    // Constructs an empty scheduler.
    Timer_Scheduler()
      noexcept;

  public:
    Timer_Scheduler(const Timer_Scheduler&) = delete;
    Timer_Scheduler& operator=(const Timer_Scheduler&) & = delete;
    ~Timer_Scheduler();

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
    insert_weak(const shptr<Abstract_Timer>& timer, milliseconds delay, milliseconds period);
  };

}  // namespace poseidon
#endif
