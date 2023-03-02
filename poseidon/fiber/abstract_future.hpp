// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FUTURE_
#define POSEIDON_FIBER_ABSTRACT_FUTURE_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Future
  {
  private:
    friend class Fiber_Scheduler;

    mutable plain_mutex m_init_mutex;
    atomic_acq_rel<Future_State> m_state;
    vector<wkptr<atomic_relaxed<steady_time>>> m_waiters;

  protected:
    // Constructs an empty future.
    explicit
    Abstract_Future() noexcept;

  protected:
    // This callback is invoked by `do_try_set_state()` below.
    virtual
    void
    do_on_future_state_change(Future_State new_state, void* param) = 0;

    // Try updating the future state. If the current future state is not
    // `future_state_empty`, this function returns immediately. Otherwise,
    // `do_on_future_state_change(new_state, param)` is invoked, and if it
    // returns normally, `m_state` is updated to `new_state`. If an
    // exception is thrown, there is no effect. `new_state` shall not be
    // `future_state_empty`.
    bool
    do_try_set_future_state_slow(Future_State new_state, void* param);

    bool
    do_try_set_future_state(Future_State new_state, void* param)
      {
        ROCKET_ASSERT(new_state != future_state_empty);
        return (this->m_state.load() == future_state_empty)
               && this->do_try_set_future_state_slow(new_state, param);
      }

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Abstract_Future);

    // Gets the future state. This function has acquire semantics.
    Future_State
    future_state() const noexcept
      { return this->m_state.load();  }
  };

}  // namespace poseidon
#endif
