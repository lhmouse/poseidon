// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "abstract_fiber.hpp"
#include "../utils.hpp"
namespace poseidon {

Abstract_Fiber::
Abstract_Fiber()
  {
  }

Abstract_Fiber::
~Abstract_Fiber()
  {
  }

void
Abstract_Fiber::
do_on_abstract_fiber_resumed()
  {
    POSEIDON_LOG_TRACE((
        "Resumed fiber `$1` (class `$2`): state `$3`"),
        this, typeid(*this),
        this->m_state.load());
  }

void
Abstract_Fiber::
do_on_abstract_fiber_suspended()
  {
    POSEIDON_LOG_TRACE((
        "Suspended fiber `$1` (class `$2`): state `$3`"),
        this, typeid(*this),
        this->m_state.load());
  }

void
Abstract_Fiber::
yield(shR<Abstract_Future> futr_opt, milliseconds fail_timeout_override) const
  {
    if(!this->m_yield)
      POSEIDON_THROW(("Fiber not executing"));

    POSEIDON_LOG_DEBUG(("Yielding from fiber `$1` (class `$2`)"), this, typeid(*this));
    this->m_yield(this->m_sched, futr_opt, fail_timeout_override);
    POSEIDON_LOG_DEBUG(("Yielded back to fiber `$1` (class `$2`)"), this, typeid(*this));
  }

}  // namespace poseidon
