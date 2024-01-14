// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "abstract_future.hpp"
#include "../utils.hpp"
namespace poseidon {

Abstract_Future::
Abstract_Future()
  {
  }

Abstract_Future::
~Abstract_Future()
  {
  }

void
Abstract_Future::
do_abstract_future_request() noexcept
  {
    // The ready state can only be updated with `m_waiters_mutex` locked.
    vector<wkptr<atomic_relaxed<steady_time>>> waiters;
    plain_mutex::unique_lock waiters_lock;

    this->m_once.call(
      [&] {
        try {
          this->do_on_abstract_future_execute();
        }
        catch(exception& stdex) {
          POSEIDON_LOG_WARN(("Future failed: `$1` (class `$2`)\n$3"), this, typeid(*this), stdex);
          this->m_except_opt = ::std::current_exception();
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
    POSEIDON_LOG_DEBUG(("Future ready: `$1` (class `$2`)"), this, typeid(*this));
  }

void
Abstract_Future::
check_ready() const
  {
    // If initialization hasn't completed yet, fail.
    this->m_once.call(
      [&] {
        POSEIDON_THROW(("Future not ready: `$1` (class `$2`)"), this, typeid(*this));
      });

    // If initialization has failed, rethrow the exact exception.
    if(this->m_except_opt)
      ::std::rethrow_exception(this->m_except_opt);
  }

}  // namespace poseidon
