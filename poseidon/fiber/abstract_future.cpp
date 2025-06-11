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
do_on_abstract_future_finalize()
  {
  }

void
Abstract_Future::
do_abstract_future_initialize_once()
  {
    plain_mutex::unique_lock lock(this->m_init_mutex);
    if(this->m_init.load())
      return;

    try {
      // Perform initialization.
      POSEIDON_LOG_DEBUG(("Initiating `$1` (class `$2`)"), this, typeid(*this));
      this->do_on_abstract_future_initialize();
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR(("Unhandled exception: $1"), stdex);
      this->m_init_except = ::std::current_exception();
    }

    cow_vector<wkptr<atomic_relaxed<steady_time>>> waiters;
    waiters.swap(this->m_waiters);

    POSEIDON_LOG_DEBUG(("Completing `$1` (class `$2`)"), this, typeid(*this));
    this->m_init.store(true);
    lock.unlock();

    // Notify all waiters.
    for(const auto& weakp : waiters)
      if(auto timep = weakp.lock())
        timep->store(steady_clock::now());

    // Perform finalization.
    POSEIDON_LOG_DEBUG(("Finalizing `$1` (class `$2`)"), this, typeid(*this));
    POSEIDON_CATCH_EVERYTHING(this->do_on_abstract_future_finalize());
  }

void
Abstract_Future::
check_success() const
  {
    if(!this->m_init.load())
      POSEIDON_THROW((
          "Future not initialized",
          "[future `$1` (class `$2`)]"),
          this, typeid(*this));

    if(this->m_init_except)
      ::std::rethrow_exception(this->m_init_except);
  }

}  // namespace poseidon
