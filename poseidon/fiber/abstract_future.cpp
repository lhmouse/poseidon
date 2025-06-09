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
    cow_vector<wkptr<atomic_relaxed<steady_time>>> waiters;
    plain_mutex::unique_lock lock(this->m_init_mutex);

    if(this->m_init_completed.load())
      return;

    try {
      // Perform initialization.
      this->do_on_abstract_future_execute();

      POSEIDON_LOG_DEBUG((
          "Future initialization completed",
          "[future `$1` (class `$2`)]"),
          this, typeid(*this));
    }
    catch(exception& stdex) {
      POSEIDON_LOG_WARN((
          "Future initialization failed: $3",
          "[future `$1` (class `$2`)]"),
          this, typeid(*this), stdex);

      // Save the exception so it will be rethrown later.
      this->m_init_except = ::std::current_exception();
    }

    this->m_init_completed.store(true);
    waiters.swap(this->m_waiters);
    lock.unlock();

    POSEIDON_CATCH_EVERYTHING(this->do_on_abstract_future_finalize());

    // Notify all waiters.
    for(const auto& weakp : waiters)
      if(auto timep = weakp.lock())
        timep->store(steady_clock::now());
  }

void
Abstract_Future::
do_on_abstract_future_finalize()
  {
  }

void
Abstract_Future::
check_success() const
  {
    if(!this->m_init_completed.load())
      POSEIDON_THROW((
          "Future not completed",
          "[future `$1` (class `$2`)]"),
          this, typeid(*this));

    if(this->m_init_except)
      ::std::rethrow_exception(this->m_init_except);
  }

}  // namespace poseidon
