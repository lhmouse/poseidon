// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "timer_driver.hpp"
#include "../base/abstract_timer.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Queued_Timer
  {
    wkptr<Abstract_Timer> wtimer;
    steady_time next;
    milliseconds period;
  };

struct Timer_Comparator
  {
    // We have to build a minheap here.
    bool
    operator()(const Queued_Timer& lhs, const Queued_Timer& rhs) noexcept
      { return lhs.next > rhs.next;  }

    bool
    operator()(const Queued_Timer& lhs, steady_time rhs) noexcept
      { return lhs.next > rhs;  }

    bool
    operator()(steady_time lhs, const Queued_Timer& rhs) noexcept
      { return lhs > rhs.next;  }
  }
  constexpr timer_comparator;

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Timer_Driver,
  Queued_Timer);

Timer_Driver::
Timer_Driver() noexcept
  {
  }

Timer_Driver::
~Timer_Driver()
  {
  }

void
Timer_Driver::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    while(this->m_pq.empty())
      this->m_pq_avail.wait(lock);

    const steady_time now = steady_clock::now();
    if(now < this->m_pq.front().next) {
      this->m_pq_avail.wait_for(lock, this->m_pq.front().next - now);
      return;
    }

    ::std::pop_heap(this->m_pq.mut_begin(), this->m_pq.mut_end(), timer_comparator);
    steady_time next = this->m_pq.back().next;
    auto timer = this->m_pq.back().wtimer.lock();
    if(!timer) {
      // If the timer has expired, delete it.
      this->m_pq.pop_back();
      return;
    }
    else if(this->m_pq.back().period != 0s) {
      // Update the next time point and insert the timer back.
      this->m_pq.mut_back().next += this->m_pq.back().period;
      ::std::push_heap(this->m_pq.mut_begin(), this->m_pq.mut_end(), timer_comparator);
    }
    else {
      // Delete the one-shot timer.
      this->m_pq.pop_back();
    }
    recursive_mutex::unique_lock driver_lock(timer->m_driver_mutex);
    timer->m_driver = this;
    lock.unlock();

    // Execute it. Exceptions are ignored.
    POSEIDON_LOG_TRACE(("Executing timer `$1` (class `$2`)"), timer, typeid(*timer));
    POSEIDON_CATCH_EVERYTHING(timer->do_abstract_timer_on_tick(next));
    timer->m_driver = reinterpret_cast<Timer_Driver*>(-5);
  }

void
Timer_Driver::
insert(const shptr<Abstract_Timer>& timer, milliseconds delay, milliseconds period)
  {
    if(!timer)
      POSEIDON_THROW(("Null timer pointer not valid"));

    if((delay < 0h) || (delay > 24000h))
      POSEIDON_THROW(("Timer delay out of range: $1"), delay);

    if((period < 0h) || (period > 24000h))
      POSEIDON_THROW(("Timer period out of range: $1"), period);

    // Calculate the end time point.
    X_Queued_Timer elem;
    elem.wtimer = timer;
    elem.next = steady_clock::now() + delay;
    elem.period = period;

    // Insert the timer.
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    this->m_pq.emplace_back(move(elem));
    ::std::push_heap(this->m_pq.mut_begin(), this->m_pq.mut_end(), timer_comparator);
    this->m_pq_avail.notify_one();
  }

}  // namespace poseidon
