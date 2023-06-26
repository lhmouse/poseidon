// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
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

bool
Abstract_Future::
do_try_set_ready(void* param)
  {
    plain_mutex::unique_lock lock(this->m_init_mutex);

    if(this->m_ready.load())
      return false;

    // Initialize the value of this future.
    this->do_on_future_ready(param);
    ::std::atomic_thread_fence(::std::memory_order_release);
    this->m_ready.store(true);

    if(!this->m_waiters.empty()) {
      // Notify waiters.
      const steady_time now = steady_clock::now();
      do {
        auto timep = this->m_waiters.back().lock();
        if(timep)
          timep->store(now);
        this->m_waiters.pop_back();
      }
      while(!this->m_waiters.empty());
    }
    return true;
  }

}  // namespace poseidon
