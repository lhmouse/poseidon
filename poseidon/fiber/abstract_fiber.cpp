// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "abstract_fiber.hpp"
#include "../utils.hpp"
namespace poseidon {

Abstract_Fiber::
Abstract_Fiber() noexcept
  {
  }

Abstract_Fiber::
~Abstract_Fiber()
  {
  }

void
Abstract_Fiber::
do_abstract_fiber_on_resumed() noexcept
  {
    POSEIDON_LOG_TRACE(("Fiber `$1` (class `$2`) resumed"), this, typeid(*this));
  }

void
Abstract_Fiber::
do_abstract_fiber_on_suspended() noexcept
  {
    POSEIDON_LOG_TRACE(("Fiber `$1` (class `$2`) suspended"), this, typeid(*this));
  }

void
Abstract_Fiber::
yield(shptrR<Abstract_Future> futr_opt, milliseconds fail_timeout_override) const
  {
    if(!this->m_yield)
      POSEIDON_THROW(("Fiber not yieldable unless assigned to a scheduler"));

    POSEIDON_LOG_INFO(("Yielding from fiber `$1` (class `$2`)"), this, typeid(*this));
    this->m_yield(this->m_sched, this, futr_opt, fail_timeout_override);
    POSEIDON_LOG_INFO(("Yielded back to fiber `$1` (class `$2`)"), this, typeid(*this));
  }

}  // namespace poseidon
