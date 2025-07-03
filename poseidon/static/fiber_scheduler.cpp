// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "fiber_scheduler.hpp"
#include "../base/config_file.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../fiber/abstract_future.hpp"
#include "../utils.hpp"
#include <algorithm>
#include <time.h>  // clock_gettime()
#include <sys/resource.h>  // getrlimit()
#include <sys/mman.h>  // mmap(), munmap()
#include <ucontext.h>  // ucontext_t
#ifdef __SANITIZE_ADDRESS__
#  include <sanitizer/asan_interface.h>
#endif

namespace poseidon {
namespace {

struct Cached_Stack
  {
    Cached_Stack* next;
    size_t vm_size;
  };

const uint32_t s_page_size = (uint32_t) ::sysconf(_SC_PAGESIZE);
atomic<Cached_Stack*> s_stack_cache;

::stack_t
do_allocate_stack(size_t vm_size)
  {
    auto cst = s_stack_cache.xchg(nullptr);
    if(cst && cst->next)
      cst->next = s_stack_cache.xchg(cst->next);

    // Deallocate anything which another thread might have put in between,
    // or when reconfiguration was requested.
    while(cst && (cst->next || (cst->vm_size != vm_size))) {
      void* map_base = (char*) cst - s_page_size;
      size_t map_size = cst->vm_size + 2 * s_page_size;
      cst = cst->next;

      if(::munmap(map_base, map_size) != 0)
        POSEIDON_LOG_FATAL((
            "Failed to unmap stack memory: map_base `$1`, map_size `$2`",
            "[`munmap()` failed: ${errno:full}]"),
            map_base, map_size);
    }

    // If the cache has been exhausted, allocate a new block of memory from
    // the system. This is allowed to throw exceptions.
    if(!cst) {
      size_t map_size = vm_size + 2 * s_page_size;
      void* map_base = ::mmap(nullptr, map_size, PROT_NONE,
                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
      if(map_base == MAP_FAILED)
        POSEIDON_THROW((
            "Could not allocate stack memory: map_size `$1`",
            "[`mmap()` failed: ${errno:full}]"),
            map_size);

      cst = (Cached_Stack*) ((char*) map_base + s_page_size);
      ::mprotect(cst, vm_size, PROT_READ | PROT_WRITE);
      cst->vm_size = vm_size;
    }

    ::stack_t st;
    st.ss_sp = cst;
    st.ss_size = cst->vm_size;
#ifdef __SANITIZE_ADDRESS__
    ::__asan_unpoison_memory_region(cst + 1, st.ss_size - sizeof(*cst));
#endif
#ifdef ROCKET_DEBUG
    ::memset(cst, 0xD9, st.ss_size);
#endif
    return st;
  }

void
do_free_stack(::stack_t st) noexcept
  {
    auto cst = (Cached_Stack*) st.ss_sp;
    if(!st.ss_sp)
      return;

#ifdef ROCKET_DEBUG
    ::memset(cst, 0xB5, st.ss_size);
#endif
#ifdef __SANITIZE_ADDRESS__
    ::__asan_poison_memory_region(cst + 1, st.ss_size - sizeof(*cst));
#endif
    cst->next = nullptr;
    cst->vm_size = st.ss_size;

    // Put the stack back into the cache.
    cst->next = s_stack_cache.load();
    while(!s_stack_cache.cmpxchg_weak(cst->next, cst));
  }

enum Fiber_State : uint8_t
  {
    st_pending     = 0,
    st_suspended   = 1,
    st_running     = 2,
    st_terminated  = 3,
  };

struct Queued_Fiber
  {
    atomic_relaxed<steady_time> async_time;  // volatile
    shptr<Abstract_Fiber> fiber;

    wkptr<Abstract_Future> wfutr;
    steady_time yield_time;
    steady_time check_time;
    Fiber_State state = st_pending;
    ::ucontext_t sched_inner[1];
  };

struct Fiber_Comparator
  {
    // We have to build a minheap here.
    bool
    operator()(const shptr<Queued_Fiber>& lhs, const shptr<Queued_Fiber>& rhs) noexcept
      { return lhs->check_time > rhs->check_time;  }

    bool
    operator()(const shptr<Queued_Fiber>& lhs, steady_time rhs) noexcept
      { return lhs->check_time > rhs;  }

    bool
    operator()(steady_time lhs, const shptr<Queued_Fiber>& rhs) noexcept
      { return lhs > rhs->check_time;  }
  }
  constexpr s_fiber_comparator;

thread_local shptr<Queued_Fiber> s_ep;  // current fiber
thread_local ::ucontext_t s_sched_outer[1];  // yield target

#ifdef __SANITIZE_ADDRESS__
thread_local void* s_sched_asan_save;
#  define do_sanitizer_start_switch_fiber()  \
    ::__sanitizer_start_switch_fiber(  \
          &(s_sched_asan_save),   \
          s_ep->sched_inner->uc_link->uc_stack.ss_sp,  \
          s_ep->sched_inner->uc_link->uc_stack.ss_size)  // no semicolon
#  define do_sanitizer_finish_switch_fiber()  \
    ::__sanitizer_finish_switch_fiber(  \
          s_sched_asan_save,  \
          &(const_cast<const void*&>(s_sched_outer->uc_stack.ss_sp)),  \
          &(s_sched_outer->uc_stack.ss_size))  // no semicolon
#else
#  define do_sanitizer_start_switch_fiber()
#  define do_sanitizer_finish_switch_fiber()
#endif

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Fiber_Scheduler,
  Queued_Fiber);

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
do_fiber_procedure() noexcept
  {
    do_sanitizer_finish_switch_fiber();
    ROCKET_ASSERT(s_ep);

    POSEIDON_LOG_TRACE(("Starting `$1` (class `$2`)"), s_ep->fiber, typeid(*(s_ep->fiber)));
    POSEIDON_CATCH_EVERYTHING(s_ep->fiber->do_on_abstract_fiber_resumed());
    ROCKET_ASSERT(s_ep->state == st_pending);
    s_ep->state = st_running;

    POSEIDON_CATCH_EVERYTHING(s_ep->fiber->do_on_abstract_fiber_execute());

    POSEIDON_LOG_TRACE(("Terminating `$1` (class `$2`)"), s_ep->fiber, typeid(*(s_ep->fiber)));
    ROCKET_ASSERT(s_ep->state == st_running);
    s_ep->state = st_terminated;
    POSEIDON_CATCH_EVERYTHING(s_ep->fiber->do_on_abstract_fiber_suspended());

    s_ep->async_time.store(steady_clock::now());
    do_sanitizer_start_switch_fiber();
  }

POSEIDON_VISIBILITY_HIDDEN
void
Fiber_Scheduler::
do_fiber_yield_function(const shptr<Abstract_Future>& futr_opt)
  {
    ROCKET_ASSERT(s_ep);
    s_ep->wfutr = futr_opt;
    s_ep->yield_time = steady_clock::now();
    s_ep->async_time.store(s_ep->yield_time);

    if(futr_opt) {
      // Associate the future. If the future is already in the READY state,
      // don't block at all.
      plain_mutex::unique_lock futr_lock(futr_opt->m_init_mutex);
      if(futr_opt->m_init.load())
        return;

      shptr<atomic_relaxed<steady_time>> async_time_ptr(s_ep, &(s_ep->async_time));
      futr_opt->m_waiters.push_back(async_time_ptr);
    }

    POSEIDON_LOG_TRACE(("Suspending `$1` (class `$2`)"), s_ep->fiber, typeid(*(s_ep->fiber)));
    ROCKET_ASSERT(s_ep->state == st_running);
    s_ep->state = st_suspended;
    POSEIDON_CATCH_EVERYTHING(s_ep->fiber->do_on_abstract_fiber_suspended());

    do_sanitizer_start_switch_fiber();
    ::swapcontext(s_ep->sched_inner, s_sched_outer);
    do_sanitizer_finish_switch_fiber();

    POSEIDON_LOG_TRACE(("Resuming `$1` (class `$2`)"), s_ep->fiber, typeid(*(s_ep->fiber)));
    POSEIDON_CATCH_EVERYTHING(s_ep->fiber->do_on_abstract_fiber_resumed());
    ROCKET_ASSERT(s_ep->state == st_suspended);
    s_ep->state = st_running;

    if(futr_opt) {
      // Unassociate the future.
      s_ep->wfutr.reset();
      futr_opt->check_success();
    }

    if(s_ep->fiber->m_abandoned.load())
      POSEIDON_THROW(("Abandoning `$1` (class `$2`)"), s_ep->fiber, typeid(*(s_ep->fiber)));
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
    auto conf_value = conf_file.query(&"fiber.stack_vm_size");
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
    conf_value = conf_file.query(&"fiber.warn_timeout");
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

    conf_value = conf_file.query(&"fiber.fail_timeout");
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
    if(s_ep)
      ASTERIA_TERMINATE(("`Fiber_Scheduler::thread_loop()` is not reentrant"));

    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const uint32_t stack_vm_size = this->m_conf_stack_vm_size;
    const seconds warn_timeout = this->m_conf_warn_timeout;
    const seconds fail_timeout = this->m_conf_fail_timeout;
    lock.unlock();

    lock.lock(this->m_pq_mutex);
    const steady_time now = steady_clock::now();
    nanoseconds time_to_wait = 999ms;
    if(!this->m_pq.empty())
      time_to_wait = this->m_pq.front()->check_time - now;
    if(time_to_wait > 0ms) {
      // Rebuild the heap when there is nothing to do. `async_time` can be
      // modified by other threads arbitrarily, so it has to be copied to
      // somewhere safe first.
      bool remake_heap = false;
      for(const auto& ptr : this->m_pq) {
        auto async_time = ptr->async_time.load();
        if(ptr->check_time != async_time) {
          remake_heap = true;
          ptr->check_time = async_time;
        }
      }

      if(remake_heap) {
        POSEIDON_LOG_TRACE(("Remaking heap: size = `$1`"), this->m_pq.size());
        ::std::make_heap(this->m_pq.mut_begin(), this->m_pq.mut_end(), s_fiber_comparator);
        time_to_wait = this->m_pq.front()->check_time - now;
      }

      time_to_wait = ::rocket::min(time_to_wait, this->m_pq_wait * 2 + 1us);
      this->m_pq_wait = ::rocket::clamp(time_to_wait, 0ms, 500ms);
      if(this->m_pq_wait > 0ms) {
        POSEIDON_LOG_TRACE(("Going to sleep for $1"), this->m_pq_wait);
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = this->m_pq_wait.count();
        lock.unlock();
        ::nanosleep(&ts, nullptr);
        return;
      }
    }

    ::std::pop_heap(this->m_pq.mut_begin(), this->m_pq.mut_end(), s_fiber_comparator);
    auto ep = this->m_pq.back();
    if(((ep->state == st_pending) && ep->fiber->m_abandoned.load()) || (ep->state == st_terminated)) {
      this->m_pq.pop_back();
      lock.unlock();

      ep->fiber.reset();
      do_free_stack(ep->sched_inner->uc_stack);
      return;
    }

    steady_time next_check_time = min(ep->check_time + warn_timeout, ep->yield_time + fail_timeout);
    ep->async_time.cmpxchg(ep->check_time, next_check_time);
    ep->check_time = next_check_time;
    ::std::push_heap(this->m_pq.mut_begin(), this->m_pq.mut_end(), s_fiber_comparator);

    recursive_mutex::unique_lock sched_lock(ep->fiber->m_sched_mutex);
    ep->fiber->m_scheduler = this;
    ep->fiber->m_sched_yield_fn = do_fiber_yield_function;
    auto futr = ep->wfutr.lock();
    lock.unlock();

    if(futr) {
      bool should_warn = now >= ep->yield_time + warn_timeout;
      bool should_fail = now >= ep->yield_time + fail_timeout;

      if(should_warn && !should_fail)
        POSEIDON_LOG_WARN((
            "`$1` (class `$2`) has been suspended for $3"),
            ep->fiber, typeid(*(ep->fiber)), duration_cast<milliseconds>(now - ep->yield_time));

      if(should_fail)
        POSEIDON_LOG_ERROR((
            "`$1` (class `$2`) has been suspended for $3",
            "This circumstance looks permanent. Please check for deadlocks."),
            ep->fiber, typeid(*(ep->fiber)), duration_cast<milliseconds>(now - ep->yield_time));

      // Wait for the future. In case of a shutdown request or timeout, ignore
      // the future and move on anyway.
      if(!should_fail && !futr->m_init.load())
        return;
    }

    if(ep->state == st_pending) {
      POSEIDON_LOG_TRACE(("Initializing `$1` (class `$2`)"), ep->fiber, typeid(*(ep->fiber)));
      ROCKET_ASSERT(ep->sched_inner->uc_stack.ss_sp == nullptr);

      // Initialize the fiber procedure and its stack.
      ::getcontext(ep->sched_inner);
      ep->sched_inner->uc_stack = do_allocate_stack(stack_vm_size);
      ep->sched_inner->uc_link = s_sched_outer;
      ::makecontext(ep->sched_inner, do_fiber_procedure, 0);
    }

    // Start or resume this fiber.
    ROCKET_ASSERT(s_ep == nullptr);
    s_ep = ep;

    do_sanitizer_start_switch_fiber();
    ::swapcontext(s_sched_outer, s_ep->sched_inner);
    do_sanitizer_finish_switch_fiber();

    ROCKET_ASSERT(s_ep == ep);
    ep->fiber->m_scheduler = reinterpret_cast<Fiber_Scheduler*>(-5);
    ep->fiber->m_sched_yield_fn = reinterpret_cast<Abstract_Fiber::sched_yield_fn*>(-9);
    s_ep.reset();
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
launch(const shptr<Abstract_Fiber>& fiber)
  {
    if(!fiber)
      POSEIDON_THROW(("Null fiber pointer not valid"));

    // Create the management node.
    auto ep = new_sh<X_Queued_Fiber>();
    ep->fiber = fiber;
    ep->yield_time = steady_clock::now();
    ep->async_time.store(ep->yield_time);
    ep->check_time = ep->yield_time;

    // Insert it.
    plain_mutex::unique_lock lock(this->m_pq_mutex);
    this->m_pq.emplace_back(move(ep));
    ::std::push_heap(this->m_pq.mut_begin(), this->m_pq.mut_end(), s_fiber_comparator);
  }

}  // namespace poseidon
