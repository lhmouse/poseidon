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
    uint32_t m_conf_warn_timeout = 0;
    uint32_t m_conf_fail_timeout = 0;

    mutable plain_mutex m_pq_mutex;
    vector<shared_ptr<X_Queued_Fiber>> m_pq;
    ::timespec m_pq_wait[1] = { };

    mutable recursive_mutex m_sched_mutex;
    weak_ptr<X_Queued_Fiber> m_sched_self_opt;
    void* m_sched_asan_save;  // private data for address sanitizer
    ::ucontext_t m_sched_outer[1];  // yield target

  public:
    // Constructs an empty scheduler.
    explicit
    Fiber_Scheduler();

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Fiber_Scheduler);

    // Gets the current time from a monotonic clock, in milliseconds.
    // This function is thread-safe.
    static
    int64_t
    clock() noexcept;

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& file);

    // Schedules fibers.
    // This function should be called by the fiber thread repeatedly.
    void
    thread_loop();

    // Returns the number of fibers that are being scheduled.
    // This function is thread-safe.
    ROCKET_PURE
    size_t
    size() const noexcept;

    // Inserts a fiber. The scheduler will take ownership of this fiber.
    // This function is thread-safe.
    void
    insert(unique_ptr<Abstract_Fiber>&& fiber);

    // Gets the current fiber if one is being scheduled.
    // This function shall be called from the same thread as `thread_loop()`.
    ROCKET_CONST
    Abstract_Fiber*
    self_opt() const noexcept;

    // Suspends the current fiber until a future becomes satisfied. `self_opt()`
    // must not return a null pointer when this function is called. If
    // `fail_timeout_override` is non-zero, it overrides `fiber.fail_timeout`
    // in 'main.conf'. Suspension may not exceed the fail timeout.
    void
    check_and_yield(const Abstract_Fiber* self, shared_ptrR<Abstract_Future> futr_opt, uint32_t fail_timeout_override);
  };

}  // namespace poseidon
#endif
