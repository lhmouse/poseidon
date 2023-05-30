// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_timer.hpp"
#include "../base/abstract_timer.hpp"
#include "../static/timer_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<Abstract_Timer> wtimer;
    cacheline_barrier xcb_1;

    // shared fields between threads
    struct Event
      {
        steady_time time;
      };

    mutable plain_mutex mutex;
    deque<Event> events;
    bool fiber_active = false;
  };

struct Shared_cb_args
  {
    wkptr<void> wobj;
    callback_thunk_ptr<shptrR<Abstract_Timer>, Abstract_Fiber&, steady_time> thunk;
    wkptr<Event_Queue> wqueue;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Shared_cb_args m_cb;

    explicit
    Final_Fiber(const Shared_cb_args& cb)
      : m_cb(cb)
      { }

    virtual
    void
    do_abstract_fiber_on_work() override
      {
        auto cb_obj = this->m_cb.wobj.lock();
        if(!cb_obj)
          return;

        for(;;) {
          // The packet callback may stop this timer, so we have to check for
          // expiry in every iteration.
          auto queue = this->m_cb.wqueue.lock();
          if(!queue)
            return;

          auto timer = queue->wtimer.lock();
          if(!timer)
            return;

          // Pop an event and invoke the user-defined callback here in the
          // main thread. Exceptions are ignored.
          plain_mutex::unique_lock lock(queue->mutex);

          if(queue->events.empty()) {
            // Terminate now.
            queue->fiber_active = false;
            return;
          }

          ROCKET_ASSERT(queue->fiber_active);
          auto event = ::std::move(queue->events.front());
          queue->events.pop_front();
          lock.unlock();

          try {
            this->m_cb.thunk(cb_obj.get(), timer, *this, event.time);
          }
          catch(exception& stdex) {
            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy timer: $1"),
                stdex);
          }
        }
      }
  };

struct Final_Timer final : Abstract_Timer
  {
    Shared_cb_args m_cb;

    explicit
    Final_Timer(Shared_cb_args&& cb)
      : m_cb(::std::move(cb))
      { }

    virtual
    void
    do_abstract_timer_on_tick(steady_time time) override
      {
        auto queue = this->m_cb.wqueue.lock();
        if(!queue)
          return;

        // We are in the timer thread here.
        plain_mutex::unique_lock lock(queue->mutex);

        if(!queue->fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_active` if no packet is pending.
          fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_cb));
          queue->fiber_active = true;
        }

        auto& event = queue->events.emplace_back();
        event.time = time;
      }
  };

}  // namespace

struct Easy_Timer::X_Event_Queue : Event_Queue
  {
  };

Easy_Timer::
~Easy_Timer()
  {
  }

void
Easy_Timer::
start(milliseconds delay, milliseconds period)
  {
    auto queue = new_sh<X_Event_Queue>();
    Shared_cb_args cb = { this->m_cb_obj, this->m_cb_thunk, queue };
    auto timer = new_sh<Final_Timer>(::std::move(cb));
    queue->wtimer = timer;

    timer_driver.insert(timer, delay, period);
    this->m_queue = ::std::move(queue);
    this->m_timer = ::std::move(timer);
  }

void
Easy_Timer::
stop() noexcept
  {
    this->m_queue = nullptr;
    this->m_timer = nullptr;
  }

}  // namespace poseidon
