// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TIMER_
#define POSEIDON_EASY_EASY_TIMER_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

class Easy_Timer
  {
  public:
    // This is the user-defined callback, where `timer` points to an internal
    // timer object, and `now` is the time point when the timer is triggered.
    // This timer stores a copy of the callback, which is invoked accordingly in
    // the main thread. The callback object is never copied, and is allowed to
    // modify itself.
    using callback_type = shared_function<
            void
             (const shptr<Abstract_Timer>& timer,
              Abstract_Fiber& fiber,
              steady_time now)>;

  private:
    struct X_Event_Queue;
    shptr<X_Event_Queue> m_queue;
    shptr<Abstract_Timer> m_timer;

  public:
    Easy_Timer() noexcept = default;
    Easy_Timer(const Easy_Timer&) = delete;
    Easy_Timer& operator=(const Easy_Timer&) & = delete;
    ~Easy_Timer();

    // Checks whether the timer is running.
    bool
    running()
      const noexcept;

    // Starts a timer, replacing the running one. The timer callback will be
    // called after `delay` milliseconds, and then, if `period` is non-zero,
    // every `period` milliseconds. If `period` is zero, the timer will only be
    // called once.
    // If an exception is thrown, there is no effect.
    shptr<Abstract_Timer>
    start(milliseconds delay, milliseconds period, const callback_type& callback);

    shptr<Abstract_Timer>
    start(milliseconds period, const callback_type& callback);

    // Stops the timer, if one is running.
    void
    stop()
      noexcept;
  };

}  // namespace poseidon
#endif
