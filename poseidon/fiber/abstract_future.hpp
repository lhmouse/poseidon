// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FUTURE_
#define POSEIDON_FIBER_ABSTRACT_FUTURE_

#include "../fwd.hpp"
#include <rocket/once_flag.hpp>
namespace poseidon {

class Abstract_Future
  {
  private:
    friend class Fiber_Scheduler;

    mutable ::rocket::once_flag m_once;
    exception_ptr m_except_opt;

    mutable plain_mutex m_waiters_mutex;
    vector<wkptr<atomic_relaxed<steady_time>>> m_waiters;

  protected:
    // Constructs an empty future that is not ready.
    explicit
    Abstract_Future();

  protected:
    // Makes this future ready by setting an optional exception, and then notifies
    // all waiters. If this future has already been made ready, this function does
    // nothing.
    void
    do_set_ready(exception_ptr&& except_opt) noexcept;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Abstract_Future);

    // Gets the ready state. This function is not a fence.
    bool
    ready() const noexcept
      { return this->m_once.test();  }

    // Checks whether this future is ready and not exceptional. If this future is
    // not ready or an exception has been set, an exception is thrown.
    void
    check_ready() const;
  };

}  // namespace poseidon
#endif
