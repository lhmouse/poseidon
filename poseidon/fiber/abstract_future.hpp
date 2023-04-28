// This file is part of Poseidon.
// Copyleft 2023 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FUTURE_
#define POSEIDON_FIBER_ABSTRACT_FUTURE_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Future
  {
  private:
    friend class Fiber_Scheduler;

    mutable plain_mutex m_init_mutex;
    atomic_acq_rel<bool> m_ready;
    vector<wkptr<atomic_relaxed<steady_time>>> m_waiters;

  protected:
    // Constructs an empty future.
    explicit
    Abstract_Future() noexcept;

  protected:
    // This callback is invoked by `do_try_set_ready()` below.
    virtual
    void
    do_on_future_ready(void* param) = 0;

    // Wakes up all waiters. Not sure whether this will be useful for users.
    void
    do_notify_ready() noexcept;

    // Tries updating the ready state. If `m_ready` is `true`, this function
    // returns immediately. Otherwise, `do_on_future_ready(param)` is called,
    // and only after it returns normally, is `m_ready` updated to `true`. If
    // an exception is thrown, there is no effect.
    bool
    do_try_set_ready(void* param);

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Abstract_Future);

    // Gets the ready state. This function has acquire semantics.
    bool
    ready() const noexcept
      { return this->m_ready.load();  }
  };

}  // namespace poseidon
#endif
