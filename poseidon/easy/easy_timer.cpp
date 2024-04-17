// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
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

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_Timer::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    Final_Fiber(const Easy_Timer::thunk_type& thunk, shptrR<Event_Queue> queue)
      :
        m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_on_abstract_fiber_execute() override
      {
        for(;;) {
          // The packet callback may stop this timer, so we have to check for
          // expiry in every iteration.
          auto queue = this->m_wqueue.lock();
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
          auto event = move(queue->events.front());
          queue->events.pop_front();
          lock.unlock();

          try {
            this->m_thunk(timer, *this, event.time);
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
    Easy_Timer::thunk_type m_thunk;
    wkptr<Event_Queue> m_wqueue;

    Final_Timer(const Easy_Timer::thunk_type& thunk, shptrR<Event_Queue> queue)
      :
        m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_abstract_timer_on_tick(steady_time time) override
      {
        auto queue = this->m_wqueue.lock();
        if(!queue)
          return;

        // We are in the timer thread here.
        plain_mutex::unique_lock lock(queue->mutex);

        if(!queue->fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_active` if no packet is pending.
          fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_thunk, queue));
          queue->fiber_active = true;
        }

        Event_Queue::Event event;
        event.time = time;
        queue->events.push_back(move(event));
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_Timer,
  Event_Queue);

Easy_Timer::
~Easy_Timer()
  {
  }

void
Easy_Timer::
start(milliseconds delay, milliseconds period)
  {
    auto queue = new_sh<X_Event_Queue>();
    auto timer = new_sh<Final_Timer>(this->m_thunk, queue);
    queue->wtimer = timer;

    timer_driver.insert(timer, delay, period);
    this->m_queue = move(queue);
    this->m_timer = move(timer);
  }

void
Easy_Timer::
stop() noexcept
  {
    this->m_queue = nullptr;
    this->m_timer = nullptr;
  }

}  // namespace poseidon
