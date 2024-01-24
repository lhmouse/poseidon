// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_FIBER_SCHEDULER_
#define POSEIDON_STATIC_FIBER_SCHEDULER_

#include "../fwd.hpp"
#include <ucontext.h>  // ucontext_t
namespace poseidon {

class Fiber_Scheduler
  {
  private:
    struct X_Queued_Fiber;

    mutable plain_mutex m_conf_mutex;
    uint32_t m_conf_stack_vm_size = 0;
    seconds m_conf_warn_timeout = zero_duration;
    seconds m_conf_fail_timeout = zero_duration;

    mutable plain_mutex m_pq_mutex;
    vector<sh<X_Queued_Fiber>> m_pq;
    ::timespec m_pq_wait = { };

    mutable recursive_mutex m_sched_mutex;
    sh<X_Queued_Fiber> m_sched_elem;
    void* m_sched_asan_save;  // private data for address sanitizer
    ::ucontext_t m_sched_outer[1];  // yield target

  public:
    // Constructs an empty scheduler.
    explicit
    Fiber_Scheduler() noexcept;

  private:
    void
    do_fiber_function() noexcept;

    void
    do_yield(shR<Abstract_Future> futr_opt, milliseconds fail_timeout_override);

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Fiber_Scheduler);

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
    launch(shR<Abstract_Fiber> fiber);
  };

}  // namespace poseidon
#endif
