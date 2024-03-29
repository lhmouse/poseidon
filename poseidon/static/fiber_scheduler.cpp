// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
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
const uint32_t s_page_size = (uint32_t) ::sysconf(_SC_PAGESIZE);
::stack_t s_stack_pool;

::stack_t
do_alloc_stack(size_t stack_vm_size)
  {
    if(stack_vm_size > 0x7F000000)
      POSEIDON_THROW(("Invalid stack size: $1"), stack_vm_size);

    const plain_mutex::unique_lock lock(s_stack_pool_mutex);

    while(s_stack_pool.ss_sp && (s_stack_pool.ss_size != stack_vm_size)) {
      // This cached stack is not large enough. Deallocate it.
      ::stack_t ss = s_stack_pool;
      s_stack_pool = *(const ::stack_t*) ss.ss_sp;

      if(::munmap((char*) ss.ss_sp - s_page_size, ss.ss_size + s_page_size * 2) != 0)
        POSEIDON_LOG_FATAL((
            "Failed to unmap fiber stack memory: ss_sp `$1`, ss_size `$2`",
            "[`munmap()` failed: ${errno:full}]"),
            ss.ss_sp, ss.ss_size);
    }

    if(!s_stack_pool.ss_sp) {
      // The pool has been exhausted. Allocate a new stack.
      void* addr = ::mmap(nullptr, stack_vm_size + s_page_size * 2, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if(addr == MAP_FAILED)
        POSEIDON_THROW((
            "Could not allocate fiber stack memory: size `$1`",
            "[`mmap()` failed: ${errno:full}]"),
            stack_vm_size);

      s_stack_pool.ss_sp = (char*) addr + s_page_size;
      s_stack_pool.ss_size = stack_vm_size;
      ::mprotect(s_stack_pool.ss_sp, s_stack_pool.ss_size, PROT_READ | PROT_WRITE);
    }

    // Pop the first stack from the pool.
    ROCKET_ASSERT(s_stack_pool.ss_sp && (s_stack_pool.ss_size >= stack_vm_size));
    ::stack_t ss = s_stack_pool;
    s_stack_pool = *(const ::stack_t*) ss.ss_sp;
#ifdef ROCKET_DEBUG
    ::memset(ss.ss_sp, 0xE9, ss.ss_size);
#endif
    return ss;
  }

void
do_free_stack(::stack_t ss) noexcept
  {
    if(!ss.ss_sp)
      return;

    const plain_mutex::unique_lock lock(s_stack_pool_mutex);

    // Prepend the stack to the pool.
#ifdef ROCKET_DEBUG
    ::memset(ss.ss_sp, 0xCD, ss.ss_size);
#endif
    *(::stack_t*) ss.ss_sp = s_stack_pool;
    s_stack_pool = ss;
  }

enum Fiber_State : uint8_t
  {
    fiber_pending     = 0,
    fiber_suspended   = 1,
    fiber_running     = 2,
    fiber_terminated  = 3,
  };

struct Queued_Fiber
  {
    atomic_relaxed<steady_time> async_time;  // volatile
    shptr<Abstract_Fiber> fiber;

    wkptr<Abstract_Future> wfutr;
    steady_time yield_time;
    steady_time check_time;
    steady_time fail_time;
    Fiber_State state = fiber_pending;
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

POSEIDON_HIDDEN_X_STRUCT(Fiber_Scheduler, Queued_Fiber);

Fiber_Scheduler::
Fiber_Scheduler() noexcept
  {
  }

Fiber_Scheduler::
~Fiber_Scheduler()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
void
Fiber_Scheduler::
do_fiber_function() noexcept
  {
    asan_fiber_switch_finish(this->m_sched_outer->uc_stack, this->m_sched_asan_save);
    auto elem = this->m_sched_elem;
    ROCKET_ASSERT(elem);
    const auto& fiber = elem->fiber;

    POSEIDON_CATCH_EVERYTHING(fiber->do_on_abstract_fiber_resumed());
    ROCKET_ASSERT(elem->state == fiber_pending);
    elem->state = fiber_running;
    POSEIDON_LOG_TRACE(("Starting fiber `$1` (class `$2`)"), fiber, typeid(*fiber));

    POSEIDON_CATCH_EVERYTHING(fiber->do_on_abstract_fiber_execute());

    POSEIDON_LOG_TRACE(("Terminating fiber `$1` (class `$2`)"), fiber, typeid(*fiber));
    ROCKET_ASSERT(elem->state == fiber_running);
    elem->state = fiber_terminated;
    POSEIDON_CATCH_EVERYTHING(fiber->do_on_abstract_fiber_suspended());

    // Return to the scheduler.
    elem->async_time.store(steady_clock::now());
    asan_fiber_switch_start(this->m_sched_asan_save, elem->sched_inner->uc_link->uc_stack);
  }

void
Fiber_Scheduler::
reload(const Config_File& conf_file)
  {
    // Parse new configuration. Default ones are defined here.
    int64_t stack_vm_size = 0;
    int64_t warn_timeout = 15;
    int64_t fail_timeout = 300;

    // Read the stack size from configuration.
    auto conf_value = conf_file.query("fiber", "stack_vm_size");
    if(conf_value.is_integer())
      stack_vm_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `fiber.stack_vm_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if(stack_vm_size == 0) {
      // If no value or zero is specified, use the system's stack size.
      ::rlimit rlim;
      if(::getrlimit(RLIMIT_STACK, &rlim) != 0)
        POSEIDON_THROW((
            "Could not get system stack size",
            "[`getrlimit()` failed: ${errno:full}]"));

      // Hmmm this should be enough.
      stack_vm_size = (uint32_t) rlim.rlim_cur;
    }

    if((stack_vm_size < 0x10000) || (stack_vm_size > 0x7FFF0000))
      POSEIDON_THROW((
          "`fiber.stack_vm_size` value `$1` out of range",
          "[in configuration file '$2']"),
          stack_vm_size, conf_file.path());

    if(stack_vm_size & 0xFFFF)
      POSEIDON_THROW((
          "`fiber.stack_vm_size` value `$1` not a multiple of 64KiB",
          "[in configuration file '$2']"),
          stack_vm_size, conf_file.path());

    // Read scheduler timeouts inseconds.
    conf_value = conf_file.query("fiber", "warn_timeout");
    if(conf_value.is_integer())
      warn_timeout = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `fiber.warn_timeout`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((warn_timeout < 0) || (warn_timeout > 86400))
      POSEIDON_THROW((
          "`fiber.warn_timeout` value `$1` out of range",
          "[in configuration file '$2']"),
          warn_timeout, conf_file.path());

    conf_value = conf_file.query("fiber", "fail_timeout");
    if(conf_value.is_integer())
      fail_timeout = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `fiber.fail_timeout`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((fail_timeout < 0) || (fail_timeout > 86400))
      POSEIDON_THROW((
          "`fiber.fail_timeout` value `$1` out of range",
          "[in configuration file '$2']"),
          fail_timeout, conf_file.path());

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_stack_vm_size = static_cast<uint32_t>(stack_vm_size);
    this->m_conf_warn_timeout = static_cast<seconds>(warn_timeout);
    this->m_conf_fail_timeout = static_cast<seconds>(fail_timeout);
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
    const steady_time now = steady_clock::now();

    if(signal == 0) {
      int64_t timeout_ns = INT_MAX;
      if(!this->m_pq.empty()) {
        timeout_ns = duration_cast<nanoseconds>(this->m_pq.front()->check_time - now).count();
        if(timeout_ns > 0) {
          // Rebuild the heap when there is nothing to do. `async_time` can be
          // modified by other threads arbitrarily, so has to be copied to
          // somewhere safe first.
          for(const auto& elem : this->m_pq)
            elem->check_time = elem->async_time.load();

          ::std::make_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);
          POSEIDON_LOG_TRACE(("Rebuilt heap for fiber scheduler: size = $1"), this->m_pq.size());
          timeout_ns = duration_cast<nanoseconds>(this->m_pq.front()->check_time - now).count();
        }
      }

      // Calculate the duration to wait, using an exponential backoff algorithm.
      timeout_ns = min(this->m_pq_wait.tv_nsec * 9 + 7, timeout_ns);
      this->m_pq_wait.tv_nsec = clamp_cast<long>(timeout_ns, 0, 200'000000);
      if(this->m_pq_wait.tv_nsec != 0) {
        lock.unlock();

        ROCKET_ASSERT(this->m_pq_wait.tv_sec == 0);
        //POSEIDON_LOG_TRACE(("Sleeping for $1 nanoseconds..."), this->m_pq_wait.tv_nsec);
        ::nanosleep(&(this->m_pq_wait), nullptr);
        return;
      }
    }

    ::std::pop_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);
    auto elem = this->m_pq.back();
    const auto& fiber = elem->fiber;
    if(elem->state == fiber_terminated) {
      this->m_pq.pop_back();
      lock.unlock();

      POSEIDON_LOG_TRACE(("Deleting fiber `$1` (class `$2`)"), fiber, typeid(*fiber));
      elem->fiber.reset();
      ROCKET_ASSERT(elem->sched_inner->uc_stack.ss_sp != nullptr);
      do_free_stack(elem->sched_inner->uc_stack);
      return;
    }

    // Put the fiber back.
    steady_time next_check_time = min(elem->check_time + warn_timeout, elem->fail_time);
    elem->async_time.cmpxchg(elem->check_time, next_check_time);
    elem->check_time = next_check_time;
    ::std::push_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);

    recursive_mutex::unique_lock sched_lock(this->m_sched_mutex);
    auto futr = elem->wfutr.lock();
    lock.unlock();

    POSEIDON_LOG_TRACE(("Processing fiber `$1` (class `$2`)"), fiber, typeid(*fiber));
    if(futr && !futr->m_once.test()) {
      // Wait for the future. In case of a shutdown request or timeout, ignore the
      // future and move on anyway.
      if(now >= elem->yield_time + warn_timeout)
        POSEIDON_LOG_WARN((
            "Fiber `$1` (class `$2`) has been suspended for $3"),
            fiber, typeid(*fiber), duration_cast<milliseconds>(now - elem->yield_time));

      if(now >= elem->fail_time)
        POSEIDON_LOG_ERROR((
            "Fiber `$1` (class `$2`) has been suspended for $3",
            "This circumstance looks permanent. Please check for deadlocks."),
            fiber, typeid(*fiber), duration_cast<milliseconds>(now - elem->yield_time));

      if((signal == 0) && (now < elem->fail_time))
        return;
    }

    if(elem->state == fiber_pending) {
      POSEIDON_LOG_TRACE(("Initializing fiber `$1` (class `$2`)"), fiber, typeid(*fiber));
      ROCKET_ASSERT(elem->sched_inner->uc_stack.ss_sp == nullptr);

      // Initialize the fiber procedure and its stack.
      ::getcontext(elem->sched_inner);
      elem->sched_inner->uc_stack = do_alloc_stack(stack_vm_size);  // may throw
      elem->sched_inner->uc_link = this->m_sched_outer;

      int xargs[2] = { };
      Fiber_Scheduler* xthis = this;
      ::memcpy(xargs, &xthis, sizeof(xthis));

      ::makecontext(elem->sched_inner,
          (void (*)()) +[](int xarg0, int xarg1)
            {
              Fiber_Scheduler* ythis = nullptr;
              int yargs[2] = { xarg0, xarg1 };
              ::memcpy(&ythis, yargs, sizeof(ythis));

              ythis->do_fiber_function();
            },
          2,
          xargs[0], xargs[1]);
    }

    // Start or resume this fiber.
    ROCKET_ASSERT(this->m_sched_elem == nullptr);
    this->m_sched_elem = elem;
    POSEIDON_LOG_TRACE(("Resuming fiber `$1` (class `$2`)"), fiber, typeid(*fiber));

    asan_fiber_switch_start(this->m_sched_asan_save, elem->sched_inner->uc_stack);
    ::swapcontext(this->m_sched_outer, elem->sched_inner);
    asan_fiber_switch_finish(elem->sched_inner->uc_stack, this->m_sched_asan_save);

    ROCKET_ASSERT(this->m_sched_elem == elem);
    this->m_sched_elem = nullptr;
    POSEIDON_LOG_TRACE(("Suspended fiber `$1` (class `$2`)"), fiber, typeid(*fiber));
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
    elem->yield_time = steady_clock::now();
    elem->check_time = elem->yield_time;
    elem->async_time.store(elem->yield_time);

    // Insert it.
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    this->m_pq.emplace_back(move(elem));
    ::std::push_heap(this->m_pq.begin(), this->m_pq.end(), fiber_comparator);
  }

void
Fiber_Scheduler::
yield(const Abstract_Fiber& tfiber, shptrR<Abstract_Future> futr_opt, milliseconds fail_timeout_override)
  {
    auto elem = this->m_sched_elem;
    if(!elem)
      POSEIDON_THROW(("No current fiber"));

    if(elem->fiber.get() != &tfiber)
      POSEIDON_THROW(("Fiber not being scheduled"));

    if(futr_opt) {
      plain_mutex::unique_lock lock(this->m_conf_mutex);
      const seconds warn_timeout = this->m_conf_warn_timeout;
      const seconds fail_timeout = this->m_conf_fail_timeout;
      lock.unlock();

      // Associate the future. If the future is already in the READY state,
      // don't block at all.
      lock.lock(futr_opt->m_waiters_mutex);
      if(futr_opt->m_once.test())
        return;

      shptr<atomic_relaxed<steady_time>> async_time_ptr(elem, &(elem->async_time));
      milliseconds real_fail_timeout = fail_timeout;

      // Clamp the timeout for safety.
      if(fail_timeout_override != 0s)
        real_fail_timeout = clamp(fail_timeout_override, 0h, 1h);

      elem->fail_time = elem->yield_time + real_fail_timeout;
      elem->async_time.store(min(elem->yield_time + warn_timeout, elem->fail_time));
      futr_opt->m_waiters.emplace_back(async_time_ptr);
      lock.unlock();
    }

    // Set re-scheduling parameters.
    const auto& fiber = elem->fiber;

    elem->wfutr = futr_opt;
    elem->yield_time = steady_clock::now();
    elem->fail_time = elem->yield_time;
    elem->async_time.store(elem->yield_time);

    POSEIDON_LOG_DEBUG(("Suspending fiber `$1` (class `$2`)"), fiber, typeid(*fiber));
    ROCKET_ASSERT(elem->state == fiber_running);
    elem->state = fiber_suspended;
    POSEIDON_CATCH_EVERYTHING(fiber->do_on_abstract_fiber_suspended());

    asan_fiber_switch_start(this->m_sched_asan_save, this->m_sched_outer->uc_stack);
    ::swapcontext(elem->sched_inner, this->m_sched_outer);
    asan_fiber_switch_finish(this->m_sched_outer->uc_stack, this->m_sched_asan_save);

    elem->wfutr.reset();

    POSEIDON_CATCH_EVERYTHING(fiber->do_on_abstract_fiber_resumed());
    ROCKET_ASSERT(elem->state == fiber_suspended);
    elem->state = fiber_running;
    POSEIDON_LOG_DEBUG(("Resumed fiber `$1` (class `$2`)"), fiber, typeid(*fiber));
  }

}  // namespace poseidon
