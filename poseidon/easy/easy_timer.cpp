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

struct Shared_cb_args
  {
    weak_ptr<void> wobj;
    callback_thunk_ptr<int64_t> thunk;
    weak_ptr<void> wuniq;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Shared_cb_args m_cb;
    int64_t m_now;

    explicit
    Final_Fiber(const Shared_cb_args& cb, int64_t now)
      : m_cb(cb), m_now(now)  { }

    virtual
    void
    do_abstract_fiber_on_work() override
      {
        auto cb_obj = this->m_cb.wobj.lock();
        if(!cb_obj)
          return;

        if(this->m_cb.wuniq.expired())
          return;

        try {
          // We are in the main thread, so invoke the user-defined callback.
          this->m_cb.thunk(cb_obj.get(), this->m_now);
        }
        catch(exception& stdex) {
          POSEIDON_LOG_ERROR((
              "Unhandled exception thrown from easy timer: $1"),
              stdex);
        }
      }
  };

struct Final_Timer final : Abstract_Timer
  {
    Shared_cb_args m_cb;

    explicit
    Final_Timer(Shared_cb_args&& cb)
      : m_cb(::std::move(cb))  { }

    virtual
    void
    do_abstract_timer_on_tick(int64_t now) override
      {
        if(this->m_cb.wuniq.expired())
          return;

        // We are in the timer thread here, so create a new fiber.
        fiber_scheduler.insert(::std::make_unique<Final_Fiber>(this->m_cb, now));
      }
  };

}  // namespace

Easy_Timer::
~Easy_Timer()
  {
  }

void
Easy_Timer::
start(int64_t delay, int64_t period)
  {
    this->m_uniq = ::std::make_shared<int>();
    Shared_cb_args cb = { this->m_cb_obj, this->m_cb_thunk, this->m_uniq };
    this->m_timer = ::std::make_shared<Final_Timer>(::std::move(cb));
    timer_driver.insert(this->m_timer, delay, period);
  }

void
Easy_Timer::
stop() noexcept
  {
    this->m_uniq = nullptr;
    this->m_timer = nullptr;
  }

}  // namespace poseidon
