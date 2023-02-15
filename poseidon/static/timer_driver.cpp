// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "timer_driver.hpp"
#include "../base/abstract_timer.hpp"
#include "../utils.hpp"
#include <time.h>
namespace poseidon {
namespace {

struct Queued_Timer
  {
    weak_ptr<Abstract_Timer> wtimer;
    uint64_t serial;
    milliseconds next;
    milliseconds period;
  };

struct Timer_Comparator
  {
    // We have to build a minheap here.
    bool
    operator()(const Queued_Timer& lhs, const Queued_Timer& rhs) noexcept
      { return lhs.next > rhs.next;  }

    bool
    operator()(const Queued_Timer& lhs, milliseconds rhs) noexcept
      { return lhs.next > rhs;  }

    bool
    operator()(milliseconds lhs, const Queued_Timer& rhs) noexcept
      { return lhs > rhs.next;  }
  }
  constexpr timer_comparator;

}  // namespace

struct Timer_Driver::X_Queued_Timer : Queued_Timer
  {
  };

Timer_Driver::
Timer_Driver()
  {
    // Generate a random serial.
    this->m_serial = (uint64_t) ::random();
  }

Timer_Driver::
~Timer_Driver()
  {
  }

milliseconds
Timer_Driver::
clock() noexcept
  {
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    int64_t count = ts.tv_sec * 1000LL + (uint32_t) ts.tv_nsec / 1000000U;
    return (milliseconds) count;
  }

void
Timer_Driver::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    while(this->m_pq.empty())
      this->m_pq_avail.wait(lock);

    const auto now = this->clock();
    ROCKET_ASSERT(this->m_pq.front().next > milliseconds(0));
    if(now < this->m_pq.front().next) {
      this->m_pq_avail.wait_for(lock, this->m_pq.front().next - now);
      return;
    }

    ::std::pop_heap(this->m_pq.begin(), this->m_pq.end(), timer_comparator);
    auto timer = this->m_pq.back().wtimer.lock();
    Async_State next_state;
    if(!timer || (this->m_pq.back().serial != timer->m_serial)) {
      // If the element has been invalidated, delete it.
      this->m_pq.pop_back();
      return;
    }
    else if(this->m_pq.back().period != milliseconds(0)) {
      // Update the next time point and insert the timer back.
      this->m_pq.back().next += this->m_pq.back().period;
      ::std::push_heap(this->m_pq.begin(), this->m_pq.end(), timer_comparator);
      next_state = async_state_suspended;
    }
    else {
      // Delete the one-shot timer.
      this->m_pq.pop_back();
      next_state = async_state_finished;
    }
    lock.unlock();

    // Execute it.
    // Exceptions are ignored.
    POSEIDON_LOG_TRACE(("Executing timer `$1` (class `$2`)"), timer, typeid(*timer));
    timer->m_state.store(async_state_running);

    try {
      timer->do_abstract_timer_on_tick(now);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Unhandled exception thrown from timer: $1",
          "[timer class `$2`]"),
          stdex, typeid(*timer));
    }

    ROCKET_ASSERT(timer->m_state.load() == async_state_running);
    timer->m_state.store(next_state);
  }

void
Timer_Driver::
insert(shared_ptrR<Abstract_Timer> timer, milliseconds delay, milliseconds period)
  {
    if(!timer)
      POSEIDON_THROW(("Null timer pointer not valid"));

    if((delay < milliseconds(0)) || (delay > milliseconds(0xFF'FFFFFFFF)))
      POSEIDON_THROW(("Timer delay out of range: $1"), delay);

    if((period < milliseconds(0)) || (period > milliseconds(0xFF'FFFFFFFF)))
      POSEIDON_THROW(("Timer period out of range: $1"), period);

    // Calculate the end time point.
    X_Queued_Timer elem;
    elem.wtimer = timer;
    elem.next = this->clock() + delay;
    elem.period = period;

    // Insert the timer.
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    elem.serial = ++ this->m_serial;
    timer->m_serial = elem.serial;
    this->m_pq.emplace_back(::std::move(elem));
    ::std::push_heap(this->m_pq.begin(), this->m_pq.end(), timer_comparator);
    this->m_pq_avail.notify_one();
  }

}  // namespace poseidon
