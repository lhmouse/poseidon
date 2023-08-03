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
        this->m_except_opt = ::std::move(except_opt);

        // Notify all waiters.
        steady_time now = steady_clock::now();
        plain_mutex::unique_lock lock(this->m_waiters_mutex);

        while(!this->m_waiters.empty()) {
          auto timep = this->m_waiters.back().lock();
          if(timep)
            timep->store(now);

          now += 1ns;
          this->m_waiters.pop_back();
        }

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
