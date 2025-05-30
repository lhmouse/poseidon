// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TIMER_
#define POSEIDON_EASY_EASY_TIMER_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

class Easy_Timer
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using callback_type =
      ::rocket::shared_function<
        void (
          shptrR<Abstract_Timer>,  // timer
          Abstract_Fiber&,         // fiber for current callback
          steady_time              // time of trigger
        )>;

  private:
    callback_type m_callback;

    struct X_Event_Queue;
    shptr<X_Event_Queue> m_queue;
    shptr<Abstract_Timer> m_timer;

  public:
    // Constructs a timer. The argument shall be an invocable object taking
    // `(shptrR<Abstract_Timer> timer, Abstract_Fiber& fiber, steady_time time)`.
    // This timer stores a copy of the callback, which is invoked accordingly in
    // the main thread. The callback object is never copied, and is allowed to
    // modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(callback_type::is_viable<xCallback>::value)>
    explicit Easy_Timer(xCallback&& cb)
      :
        m_callback(forward<xCallback>(cb))
      { }

  public:
    Easy_Timer(const Easy_Timer&) = delete;
    Easy_Timer& operator=(const Easy_Timer&) & = delete;
    ~Easy_Timer();

    // Starts a timer if none is running, or resets the running one. The timer
    // callback will be called after `delay` milliseconds, and then, if `period`
    // is non-zero, periodically every `period` milliseconds. If `period` is
    // zero, the timer will only be called once.
    // If an exception is thrown, there is no effect.
    void
    start(milliseconds delay, milliseconds period);

    // Stops the timer, if one is running.
    void
    stop() noexcept;
  };

}  // namespace poseidon
#endif
