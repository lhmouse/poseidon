// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_timer.hpp"
#include "../base/abstract_timer.hpp"
#include "../static/timer_scheduler.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
#include <deque>
namespace poseidon {
namespace {

struct Event
  {
    steady_time time;
  };

struct Event_Queue
  {
    // read-only fields; no locking needed
    wkptr<Abstract_Timer> wtimer;
    cacheline_barrier xcb_1;

    // shared fields between threads
    mutable plain_mutex mutex;
    ::std::deque<Event> events;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_Timer::callback_type m_callback;
    wkptr<Event_Queue> m_wqueue;

    Final_Fiber(const Easy_Timer::callback_type& callback,
                const shptr<Event_Queue>& queue)
      :
        m_callback(callback), m_wqueue(queue)
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
            // Call the user-defined callback.
            this->m_callback(timer, *this, event.time);
          }
          catch(exception& stdex) {
            // Ignore this exception.
            POSEIDON_LOG_ERROR(("Unhandled exception: $1"), stdex);
          }
        }
      }
  };

struct Final_Timer final : Abstract_Timer
  {
    Easy_Timer::callback_type m_callback;
    wkptr<Event_Queue> m_wqueue;

    Final_Timer(const Easy_Timer::callback_type& callback,
                const shptr<Event_Queue>& queue)
      :
        m_callback(callback), m_wqueue(queue)
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
          fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_callback, queue));
          queue->fiber_active = true;
        }

        Event event;
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

bool
Easy_Timer::
running() const noexcept
  {
    return this->m_timer != nullptr;
  }

shptr<Abstract_Timer>
Easy_Timer::
start(milliseconds delay, milliseconds period, const callback_type& callback)
  {
    auto queue = new_sh<X_Event_Queue>();
    auto timer = new_sh<Final_Timer>(callback, queue);
    queue->wtimer = timer;

    timer_scheduler.insert_weak(timer, delay, period);
    this->m_queue = move(queue);
    this->m_timer = timer;
    return timer;
  }

shptr<Abstract_Timer>
Easy_Timer::
start(milliseconds period, const callback_type& callback)
  {
    return this->start(period, period, callback);
  }

void
Easy_Timer::
stop() noexcept
  {
    this->m_queue = nullptr;
    this->m_timer = nullptr;
  }

}  // namespace poseidon
