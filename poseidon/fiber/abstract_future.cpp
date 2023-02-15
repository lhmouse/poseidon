// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "abstract_future.hpp"
#include "../static/fiber_scheduler.hpp"
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
do_try_set_future_state_slow(Future_State new_state, void* param)
  {
    ROCKET_ASSERT(new_state != future_state_empty);
    plain_mutex::unique_lock lock(this->m_init_mutex);

    if(this->m_state.load() != future_state_empty)
      return false;

    // Perform initialization.
    this->do_on_future_state_change(new_state, param);
    this->m_state.store(new_state);

    if(!this->m_waiters.empty()) {
      const auto now = Fiber_Scheduler::clock();
      shared_ptr<atomic_relaxed<milliseconds>> async_time_ptr;

      // Wake up all waiters. This will not throw exceptions.
      for(const auto& wp : this->m_waiters)
        if((async_time_ptr = wp.lock()) != nullptr)
          async_time_ptr->store(now);
    }
    return true;
  }

}  // namespace poseidon
