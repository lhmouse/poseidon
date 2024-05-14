// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FUTURE_
#define POSEIDON_FIBER_ABSTRACT_FUTURE_

#include "../fwd.hpp"
#include "../base/abstract_task.hpp"
#include <rocket/once_flag.hpp>
namespace poseidon {

class Abstract_Future
  :
    public Abstract_Task
  {
  private:
    friend class Fiber_Scheduler;

    mutable ::rocket::once_flag m_once;
    ::std::exception_ptr m_except;

    mutable plain_mutex m_waiters_mutex;
    cow_vector<wkptr<atomic_relaxed<steady_time>>> m_waiters;

  protected:
    // Constructs an empty future that has not completed.
    Abstract_Future() noexcept;

  protected:
    // Requests the asynchronous work and sets the completion state. If the work
    // has not been done yet, this function calls `do_on_abstract_future_execute()`
    // do the work; otherwise, this function returns immediately.
    virtual
    void
    do_on_abstract_task_execute() noexcept override;

    // This callback is invoked by `do_abstract_future_request()` and is intended
    // to be overriden by derived classes to do the asynchronous work. If this
    // function throws an exception, it will be copied into `m_except` which
    // can be examined with `check_result()`.
    virtual
    void
    do_on_abstract_future_execute() = 0;

    // This callback is invoked by `do_abstract_future_request()` after the work
    // has been completed (either successfully or exceptionally) and all blocking
    // fibers have been released. It's much like the `finally` block in Java,
    // except that it shall not throw exceptions.
    virtual
    void
    do_on_abstract_future_finalize() noexcept;

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
      { return this->m_once.test() && !this->m_except;  }

    // Checks whether this future has completed with no exception being thrown.
    void
    check_success() const;
  };

}  // namespace poseidon
#endif
