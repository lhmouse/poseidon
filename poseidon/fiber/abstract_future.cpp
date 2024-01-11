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
do_set_ready(exception_ptr&& except_opt) noexcept
  {
    this->m_once.call(
      [&] {
        // Set the exception pointer to indicate success or failure.
        this->m_except_opt = move(except_opt);

        // Notify all waiters.
        steady_time now = steady_clock::now();
        vector<wkptr<atomic_relaxed<steady_time>>> waiters;
        plain_mutex::unique_lock waiters_lock(this->m_waiters_mutex);

        waiters.swap(this->m_waiters);
        for(uint32_t k = 0;  k != waiters.size();  ++k)
          if(auto timep = waiters[k].lock())
            timep->store(now + steady_clock::time_point::duration(k));

        POSEIDON_LOG_DEBUG(("Future `$1` ready (class `$2`)"), this, typeid(*this));
      });
  }

void
Abstract_Future::
check_ready() const
  {
    this->m_once.call(
      [&] {
        // Throw an exception if initialization has not taken place.
        if(this->m_except_opt)
          ::std::rethrow_exception(this->m_except_opt);

        POSEIDON_THROW(("Future `$1` not ready (class `$2`)"), this, typeid(*this));
      });
  }

}  // namespace poseidon
