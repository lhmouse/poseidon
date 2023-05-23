// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "fiber_scheduler.hpp"
#include "../base/config_file.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../fiber/abstract_future.hpp"
#include "../third/asan_fwd.hpp"
#include "../utils.hpp"
#include <time.h>  // clock_gettime()
#include <sys/resource.h>  // getrlimit()
#include <sys/mman.h>  // mmap(), munmap()

namespace poseidon {
namespace {

plain_mutex s_stack_pool_mutex;
::stack_t s_stack_pool;

void
do_free_stack(::stack_t ss) noexcept
  {
    if(!ss.ss_sp)
      return;

    if(::munmap((char*) ss.ss_sp - 0x1000, ss.ss_size + 0x2000) != 0)
      POSEIDON_LOG_FATAL((
          "Failed to unmap fiber stack memory `$2` of size `$3`",
          "[`munmap()` failed: $1]"),
          format_errno(), ss.ss_sp, ss.ss_size);
  }

::stack_t
do_alloc_stack(size_t stack_vm_size)
  {
    if(stack_vm_size > 0x7FFF0000)
      POSEIDON_THROW(("Invalid stack size: $1"), stack_vm_size);

    for(;;) {
      // Try popping a cached one.
      plain_mutex::unique_lock lock(s_stack_pool_mutex);
      ::stack_t ss = s_stack_pool;
      if(!ss.ss_sp)
        break;

      ::memcpy(&s_stack_pool, ss.ss_sp, sizeof(ss));
      lock.unlock();

      // If it is large enough, return it.
      if(ss.ss_size >= stack_vm_size)
        return ss;

      // Otherwise, free it and try the next one.
      do_free_stack(ss);
    }

    // Allocate a new stack.
    ::stack_t ss;
    ss.ss_size = stack_vm_size;
    ss.ss_sp = ::mmap(nullptr, ss.ss_size + 0x2000, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(ss.ss_sp == (void*) -1)
      POSEIDON_THROW((
          "Could not allocate fiber stack memory of size `$2`",
          "[`mmap()` failed: $1]"),
          format_errno(), ss.ss_size);

    // Adjust the pointer to writable memory, and make it so.
    ss.ss_sp = (char*) ss.ss_sp + 0x1000;
    ::mprotect(ss.ss_sp, ss.ss_size, PROT_READ | PROT_WRITE);
    return ss;
  }

void
do_pool_stack(::stack_t ss) noexcept
  {
    if(!ss.ss_sp)
      return;

    // Prepend the stack to the list.
    plain_mutex::unique_lock lock(s_stack_pool_mutex);
    ::memcpy(ss.ss_sp, &s_stack_pool, sizeof(ss));
    s_stack_pool = ss;
  }

struct Queued_Fiber
  {
    shptr<Abstract_Fiber> fiber;
    atomic_relaxed<steady_time> async_time;  // volatile

    wkptr<Abstract_Future> wfutr;
    steady_time yield_time, check_time, fail_time;
    ::ucontext_t sched_inner[1];
  };

struct Fiber_Comparator
  {
    // We have to build a minheap here.
    bool
    operator()(shptrR<Queued_Fiber> lhs, shptrR<Queued_Fiber> rhs) noexcept
      { return lhs->check_time > rhs->check_time;  }

    bool
    operator()(shptrR<Queued_Fiber> lhs, steady_time rhs) noexcept
      { return lhs->check_time > rhs;  }

    bool
    operator()(steady_time lhs, shptrR<Queued_Fiber> rhs) noexcept
      { return lhs > rhs->check_time;  }
  }
  constexpr fiber_comparator;

}  // namespace

struct Fiber_Scheduler::X_Queued_Fiber : Queued_Fiber
  {
  };

Fiber_Scheduler::
Fiber_Scheduler()
  {
  }

Fiber_Scheduler::
~Fiber_Scheduler()
  {
  }

void
Fiber_Scheduler::
do_fiber_function() noexcept
  {
    asan_fiber_switch_finish(this->m_sched_asan_save);
    const auto& elem = this->m_sched_elem;
    ROCKET_ASSERT(elem);

    // Change the fiber state from PENDING to RUNNING.
    elem->fiber->do_abstract_fiber_on_resumed();
    ROCKET_ASSERT(elem->fiber->m_state.load() == async_state_pending);
    elem->fiber->m_state.store(async_state_running);
    POSEIDON_LOG_TRACE(("Starting fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));

    try {
      elem->fiber->do_abstract_fiber_on_work();
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Unhandled exception thrown from fiber: $1",
          "[fiber class `$2`]"),
          stdex, typeid(*(elem->fiber)));
    }

    // Change the fiber state from RUNNING to FINISHED.
    ROCKET_ASSERT(elem->fiber->m_state.load() == async_state_running);
    elem->fiber->m_state.store(async_state_finished);
    POSEIDON_LOG_TRACE(("Terminating fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));
    elem->fiber->do_abstract_fiber_on_suspended();

    // Return to the scheduler.
    elem->async_time.store(time_point_cast<milliseconds>(steady_clock::now()));
    asan_fiber_switch_start(this->m_sched_asan_save, elem->sched_inner->uc_link);
  }

void
Fiber_Scheduler::
do_yield(shptrR<Abstract_Future> futr_opt, milliseconds fail_timeout_override)
  {
    const auto& elem = this->m_sched_elem;
    ROCKET_ASSERT(elem);

    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const seconds warn_timeout = this->m_conf_warn_timeout;
    const seconds fail_timeout = this->m_conf_fail_timeout;
    lock.unlock();

    // Set re-scheduling parameters.
    elem->yield_time = time_point_cast<milliseconds>(steady_clock::now());
    elem->fail_time = elem->yield_time;
    elem->async_time.store(elem->yield_time);

    if(futr_opt) {
      // Associate the future. If the future is already in the READY state,
      // don't block at all.
      lock.lock(futr_opt->m_init_mutex);
      if(futr_opt->ready())
        return;

      shptr<atomic_relaxed<steady_time>> async_time_ptr(elem, &(elem->async_time));
      milliseconds real_fail_timeout = fail_timeout;

      // Clamp the timeout for safety.
      if(fail_timeout_override != zero_duration)
        real_fail_timeout = clamp(fail_timeout_override, (hours) 0, (hours) 1);

      elem->fail_time = elem->yield_time + real_fail_timeout;
      elem->async_time.store(min(elem->yield_time + warn_timeout, elem->fail_time));
      futr_opt->m_waiters.emplace_back(async_time_ptr);
    }
    lock.unlock();

    // Suspend the current fiber...
    elem->fiber->do_abstract_fiber_on_suspended();
    elem->wfutr = futr_opt;
    ROCKET_ASSERT(elem->fiber->m_state.load() == async_state_running);
    elem->fiber->m_state.store(async_state_suspended);
    POSEIDON_LOG_TRACE(("Suspending fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));

    asan_fiber_switch_start(this->m_sched_asan_save, this->m_sched_outer);
    ::swapcontext(elem->sched_inner, this->m_sched_outer);
    asan_fiber_switch_finish(this->m_sched_asan_save);

    // ... and return here.
    POSEIDON_LOG_TRACE(("Resumed fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));
    ROCKET_ASSERT(elem->fiber->m_state.load() == async_state_suspended);
    elem->fiber->m_state.store(async_state_running);
    elem->wfutr.reset();
    elem->fiber->do_abstract_fiber_on_resumed();
  }

void
Fiber_Scheduler::
reload(const Config_File& file)
  {
    // Parse new configuration. Default ones are defined here.
    int64_t stack_vm_size = 0;
    int64_t warn_timeout = 15;
    int64_t fail_timeout = 300;

    // Read the stack size from configuration.
    auto value = file.query("fiber", "stack_vm_size");
    if(value.is_integer())
      stack_vm_size = value.as_integer();
    else if(!value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `fiber.stack_vm_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          value, file.path());

    if(stack_vm_size == 0) {
      // If no value or zero is specified, use the system's stack size.
      ::rlimit rlim;
      if(::getrlimit(RLIMIT_STACK, &rlim) != 0)
        POSEIDON_THROW((
            "Could not get system stack size",
            "[`getrlimit()` failed: $1]"),
            format_errno());

      // Hmmm this should be enough.
      stack_vm_size = (uint32_t) rlim.rlim_cur;
    }

    if((stack_vm_size < 0x10000) || (stack_vm_size > 0x7FFF0000))
      POSEIDON_THROW((
          "`fiber.stack_vm_size` value `$1` out of range",
          "[in configuration file '$2']"),
          stack_vm_size, file.path());

    if(stack_vm_size & 0xFFFF)
      POSEIDON_THROW((
          "`fiber.stack_vm_size` value `$1` not a multiple of 64KiB",
          "[in configuration file '$2']"),
          stack_vm_size, file.path());

    // Read scheduler timeouts inseconds.
    value = file.query("fiber", "warn_timeout");
    if(value.is_integer())
      warn_timeout = value.as_integer();
    else if(!value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `fiber.warn_timeout`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          value, file.path());

    if((warn_timeout < 0) || (warn_timeout > 86400))
      POSEIDON_THROW((
          "`fiber.warn_timeout` value `$1` out of range",
          "[in configuration file '$2']"),
          warn_timeout, file.path());

    value = file.query("fiber", "fail_timeout");
    if(value.is_integer())
      fail_timeout = value.as_integer();
    else if(!value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `fiber.fail_timeout`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          value, file.path());

    if((fail_timeout < 0) || (fail_timeout > 86400))
      POSEIDON_THROW((
          "`fiber.fail_timeout` value `$1` out of range",
          "[in configuration file '$2']"),
          fail_timeout, file.path());

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_stack_vm_size = (uint32_t) stack_vm_size;
    this->m_conf_warn_timeout = (seconds) warn_timeout;
    this->m_conf_fail_timeout = (seconds) fail_timeout;
  }

void
Fiber_Scheduler::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const size_t stack_vm_size = this->m_conf_stack_vm_size;
    const seconds warn_timeout = this->m_conf_warn_timeout;
    lock.unlock();

    lock.lock(this->m_pq_mutex);
    const int signal = exit_signal.load();
    const auto now = time_point_cast<milliseconds>(steady_clock::now());

    if(signal == 0) {
      if(!this->m_pq.empty() && (now < this->m_pq.front()->check_time)) {
        // Rebuild the heap when there is nothing to do. `async_time` can be
        // modified by other threads arbitrarily, so has to be copied to
        // somewhere safe first.
        for(const auto& elem : this->m_pq)
          elem->check_time = elem->async_time.load();

        ::std::make_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);
        POSEIDON_LOG_TRACE(("Rebuilt heap for fiber scheduler: size = $1"), this->m_pq.size());
      }

      // Calculate the number of nanoseconds to sleep.
      this->m_pq_wait->tv_nsec = min(
             this->m_pq_wait->tv_nsec * 2 + 1,  // binary exponential backoff
             200'000000L,  // maximum value
             this->m_pq.empty() ? LONG_MAX
                : clamp((this->m_pq.front()->check_time - now).count(), 0, 10000) * 1000000L);

      if(this->m_pq_wait->tv_nsec != 0) {
        lock.unlock();

        ROCKET_ASSERT(this->m_pq_wait->tv_sec == 0);
        ::nanosleep(this->m_pq_wait, nullptr);
        return;
      }
    }

    ::std::pop_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);
    auto elem = this->m_pq.back();
    if(elem->fiber->m_state.load() == async_state_finished) {
      this->m_pq.pop_back();
      lock.unlock();

      POSEIDON_LOG_TRACE(("Deleting fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));
      elem->fiber.reset();
      do_pool_stack(elem->sched_inner->uc_stack);
      return;
    }

    // Put the fiber back.
    steady_time next_check_time = min(elem->check_time + warn_timeout, elem->fail_time);
    elem->async_time.cmpxchg(elem->check_time, next_check_time);
    elem->check_time = next_check_time;
    ::std::push_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);

    recursive_mutex::unique_lock sched_lock(this->m_sched_mutex);
    elem->fiber->m_sched = this;
    lock.unlock();

    POSEIDON_LOG_TRACE((
        "Processing fiber `$1` (class `$2`): state = $3"),
        elem->fiber, typeid(*(elem->fiber)), elem->fiber->m_state.load());

    // Check timeouts.
    auto futr = elem->wfutr.lock();

    if(now >= elem->yield_time + warn_timeout)
      POSEIDON_LOG_WARN((
          "Fiber `$1` (class `$2`) has been suspended for $3"),
          elem->fiber, typeid(*(elem->fiber)), now - elem->yield_time);

    if((now >= elem->fail_time) && futr && !futr->ready())
      POSEIDON_LOG_ERROR((
          "Fiber `$1` (class `$2`) has been suspended for $3",
          "This circumstance looks permanent. Please check for deadlocks."),
          elem->fiber, typeid(*(elem->fiber)), now - elem->yield_time);

    if((now < elem->fail_time) && (signal == 0) && futr && !futr->ready())
      return;

    if(!elem->sched_inner->uc_stack.ss_sp) {
      POSEIDON_LOG_TRACE(("Initializing fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));

      // Initialize the fiber procedure and its stack.
      ::getcontext(elem->sched_inner);
      elem->sched_inner->uc_stack = do_alloc_stack(stack_vm_size);  // may throw
      elem->sched_inner->uc_link = this->m_sched_outer;

      int xargs[2];
      Fiber_Scheduler* xthis = this;
      ::memcpy(xargs, &xthis, sizeof(xthis));

      ::makecontext(elem->sched_inner,
          (void (*)()) +[](int xarg0, int xarg1)
            {
              Fiber_Scheduler* ythis;
              int yargs[2] = { xarg0, xarg1 };
              ::memcpy(&ythis, yargs, sizeof(ythis));

              ythis->do_fiber_function();
            },
          2, xargs[0], xargs[1]);

      elem->fiber->m_yield =
          +[](Fiber_Scheduler* ythis, const Abstract_Fiber* yfiber, shptrR<Abstract_Future> yfutr, milliseconds ytimeout)
            {
              if(!ythis->m_sched_elem->fiber)
                POSEIDON_THROW(("No current fiber"));
              else if(yfiber != ythis->m_sched_elem->fiber.get())
                POSEIDON_THROW(("Cannot yield execution outside current fiber"));

              ythis->do_yield(yfutr, ytimeout);
            };
    }

    // Start or resume this fiber.
    ROCKET_ASSERT(this->m_sched_elem == nullptr);
    this->m_sched_elem = elem;
    POSEIDON_LOG_TRACE(("Resuming fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));

    asan_fiber_switch_start(this->m_sched_asan_save, elem->sched_inner);
    ::swapcontext(this->m_sched_outer, elem->sched_inner);
    asan_fiber_switch_finish(this->m_sched_asan_save);

    ROCKET_ASSERT(this->m_sched_elem == elem);
    this->m_sched_elem = nullptr;
    POSEIDON_LOG_TRACE(("Suspended fiber `$1` (class `$2`)"), elem->fiber, typeid(*(elem->fiber)));
  }

size_t
Fiber_Scheduler::
size() const noexcept
  {
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    return this->m_pq.size();
  }

void
Fiber_Scheduler::
launch(shptrR<Abstract_Fiber> fiber)
  {
    if(!fiber)
      POSEIDON_THROW(("Null fiber pointer not valid"));

    // Create the management node.
    auto elem = new_sh<X_Queued_Fiber>();
    elem->fiber = fiber;
    elem->yield_time = time_point_cast<milliseconds>(steady_clock::now());
    elem->check_time = elem->yield_time;
    elem->async_time.store(elem->yield_time);

    // Insert it.
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    this->m_pq.emplace_back(::std::move(elem));
    ::std::push_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);
  }

}  // namespace poseidon
