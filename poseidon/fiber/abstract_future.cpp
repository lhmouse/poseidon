// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_future.hpp"
#include "../utils.hpp"
namespace poseidon {

Abstract_Future::
Abstract_Future() noexcept
  {
  }

Abstract_Future::
~Abstract_Future()
  {
  }

void
Abstract_Future::
do_on_abstract_task_execute() noexcept
  {
    // The completion state can only be updated with `m_waiters_mutex` locked.
    cow_vector<wkptr<atomic_relaxed<steady_time>>> waiters;
    plain_mutex::unique_lock waiters_lock;

    this->m_once.call(
      [&] {
        try {
          this->do_on_abstract_future_execute();
        }
        catch(exception& stdex) {
          POSEIDON_LOG_WARN(("Future `$1` failed:", "$2"), typeid(*this), stdex);
          this->m_except = ::std::current_exception();
        }

        // This indicates something has been done and all waiters shall be
        // released.
        waiters_lock.lock(this->m_waiters_mutex);
      });

    if(!waiters_lock)
      return;

    // Notify all waiters.
    steady_time now = steady_clock::now();
    waiters.swap(this->m_waiters);

    for(uint32_t k = 0;  k != waiters.size();  ++k)
      if(auto timep = waiters[k].lock())
        timep->store(now + steady_clock::time_point::duration(k));

    waiters_lock.unlock();
    this->do_on_abstract_future_finalize();
    POSEIDON_LOG_TRACE(("Future `$1` completed"), typeid(*this));
  }

void
Abstract_Future::
do_on_abstract_future_finalize() noexcept
  {
  }

void
Abstract_Future::
check_success() const
  {
    // If initialization hasn't completed yet, fail.
    this->m_once.call(
      [&] {
        POSEIDON_THROW(("Future `$1` not completed"), typeid(*this));
      });

    // If initialization has failed, rethrow the exact exception.
    if(this->m_except)
      ::std::rethrow_exception(this->m_except);
  }

}  // namespace poseidon
