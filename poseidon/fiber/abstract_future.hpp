// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FUTURE_
#define POSEIDON_FIBER_ABSTRACT_FUTURE_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Future
  {
  private:
    template<typename> friend class future;;
    friend class Fiber_Scheduler;

    mutable plain_mutex m_init_mutex;
    atomic_acq_rel<Future_State> m_state;
    vector<weak_ptr<atomic_relaxed<int64_t>>> m_waiters;

  protected:
    // Constructs an empty future.
    explicit
    Abstract_Future() noexcept;

  private:
    void
    do_abstract_future_check_value(const char* type, const exception_ptr* exptr) const;

    void
    do_abstract_future_signal_nolock() noexcept;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Abstract_Future);

    // Gets the future state.
    // This function has acquire semantics. If `future_state_value` is returned,
    // a value should have been initialized by derived classes.
    Future_State
    future_state() const noexcept
      { return this->m_state.load();  }
  };

}  // namespace poseidon
#endif
