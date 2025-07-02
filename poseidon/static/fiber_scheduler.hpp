// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_FIBER_SCHEDULER_
#define POSEIDON_STATIC_FIBER_SCHEDULER_

#include "../fwd.hpp"
namespace poseidon {

class Fiber_Scheduler
  {
  private:
    struct X_Queued_Fiber;

    mutable plain_mutex m_conf_mutex;
    uint32_t m_conf_stack_vm_size = 0;
    seconds m_conf_warn_timeout = 0s;
    seconds m_conf_fail_timeout = 0s;

    mutable plain_mutex m_pq_mutex;
    cow_vector<shptr<X_Queued_Fiber>> m_pq;
    nanoseconds m_pq_wait;

  public:
    // Constructs an empty scheduler.
    Fiber_Scheduler() noexcept;

  private:
    static
    void
    do_fiber_procedure() noexcept;

    static
    void
    do_fiber_yield_function(const shptr<Abstract_Future>& futr_opt);

  public:
    Fiber_Scheduler(const Fiber_Scheduler&) = delete;
    Fiber_Scheduler& operator=(const Fiber_Scheduler&) & = delete;
    ~Fiber_Scheduler();

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& conf_file);

    // Schedules fibers.
    // This function should be called by the fiber thread repeatedly.
    void
    thread_loop();

    // Returns the number of fibers that are being scheduled.
    // This function is thread-safe.
    ROCKET_PURE
    size_t
    size() const noexcept;

    // Takes ownership of a fiber, and schedules it for execution. The fiber
    // can only be deleted after it finishes execution.
    // This function is thread-safe.
    void
    launch(const shptr<Abstract_Fiber>& fiber);
  };

}  // namespace poseidon
#endif
