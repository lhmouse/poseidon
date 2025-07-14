// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FUTURE_
#define POSEIDON_FIBER_ABSTRACT_FUTURE_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Future
  {
  private:
    friend class Fiber_Scheduler;

    atomic_acq_rel<bool> m_init;
    mutable plain_mutex m_init_mutex;
    ::std::exception_ptr m_init_except;
    cow_vector<wkptr<atomic_relaxed<steady_time>>> m_waiters;

  protected:
    // Constructs an uninitialized future.
    Abstract_Future()
      noexcept;

  protected:
    // This callback is invoked by `do_abstract_future_initialize_once()` and is
    // intended to be overriden by derived classes to do asynchronous work. If
    // an exception is thrown, it will be caught and stored into `m_except`,
    // which can be rethrown by `check_result()`.
    virtual
    void
    do_on_abstract_future_initialize()
      = 0;

    // This callback is invoked by `do_abstract_future_initialize_once()` and is
    // intended to be overriden by derived classes to do cleanup work. If an
    // exception is thrown, it is silently ignored.
    // The default implementation does nothing.
    virtual
    void
    do_on_abstract_future_finalize();

    // Requests one-time initialization. If `m_init` is `false`, this function
    // calls `do_on_abstract_future_initialize()`; if an exception is thrown, it
    // is caught and stored into `m_init_except`. Then, `m_init` is set to
    // `true` and all fibers that are blocking on this future are released.
    // Finally, `do_on_abstract_future_finalize()` is called; exceptions are
    // ignored. The entire operation is thread-safe.
    void
    do_abstract_future_initialize_once();

    // This is the out-of-line implementation of `check_success()`.
    void
    do_abstract_future_check_success()
      const;

  public:
    Abstract_Future(const Abstract_Future&) = delete;
    Abstract_Future& operator=(const Abstract_Future&) & = delete;
    virtual ~Abstract_Future();

    // Checks whether initialization has completed.
    bool
    initialized()
      const noexcept
      { return this->m_init.load();  }

    // Checks whether initialization has completed without an exception.
    bool
    successful()
      const noexcept
      { return this->m_init.load() && !this->m_init_except;  }

    // Checks whether this future has been initialized. If an exception has been
    // caught and saved, it is rethrown.
    void
    check_success()
      const
      {
        if(!this->m_init.load())
          this->do_abstract_future_check_success();
      }
  };

}  // namespace poseidon
#endif
