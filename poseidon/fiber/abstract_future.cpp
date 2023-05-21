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

void
Abstract_Future::
do_notify_ready() noexcept
  {
    if(this->m_waiters.empty())
      return;

    vector<wkptr<atomic_relaxed<steady_time>>> waiters;
    waiters.swap(this->m_waiters);
    const auto now = time_point_cast<milliseconds>(steady_clock::now());

    for(const auto& wp : waiters)
      if(auto p = wp.lock())
        p->store(now);
  }

bool
Abstract_Future::
do_try_set_ready(void* param)
  {
    plain_mutex::unique_lock lock(this->m_init_mutex);

    if(this->m_ready.load())
      return false;

    this->do_on_future_ready(param);
    this->m_ready.store(true);
    this->do_notify_ready();
    return true;
  }

}  // namespace poseidon
