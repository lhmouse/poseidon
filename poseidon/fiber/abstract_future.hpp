// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

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
    // Constructs an empty future that has not completed.
    Abstract_Future() noexcept;

  protected:
    // This callback is invoked by `do_abstract_future_request()` and is intended
    // to be overriden by derived classes to do the asynchronous work. If this
    // function throws an exception, it will be copied into `m_except_opt` which
    // can be examined with `check_result()`.
    virtual
    void
    do_on_abstract_future_execute() = 0;

    // Requests the asynchronous work and sets the completion state. If the work
    // has not been done yet, this function calls `do_on_abstract_future_execute()`
    // do the work; otherwise, this function returns immediately.
    void
    do_abstract_future_request() noexcept;

  public:
    Abstract_Future(const Abstract_Future&) = delete;
    Abstract_Future& operator=(const Abstract_Future&) & = delete;
    virtual ~Abstract_Future();

    // Gets the completion state. If this function returns `true`, then either a
    // result or an exception will have been set.
    bool
    completed() const noexcept
      { return this->m_once.test();  }

    bool
    successful() const noexcept
      { return this->m_once.test() && !this->m_except_opt;  }

    // Checks whether this future has completed with no exception being thrown.
    void
    check_success() const;
  };

}  // namespace poseidon
#endif
