// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TIMER_
#define POSEIDON_EASY_EASY_TIMER_

#include "../fwd.hpp"
namespace poseidon {

class Easy_Timer
  {
  private:
    struct X_Event_Queue;

    shptr<void> m_cb_obj;
    callback_thunk_ptr<shptrR<Abstract_Timer>, Abstract_Fiber&, steady_time> m_cb_thunk;

    shptr<X_Event_Queue> m_queue;
    shptr<Abstract_Timer> m_timer;

  public:
    // Constructs a timer. The argument shall be an invocable object taking
    // `(shptrR<Abstract_Timer> timer, Abstract_Fiber& fiber, steady_time time)`.
    // This timer stores a copy of the callback, which is invoked accordingly in
    // the main thread. The callback object is never copied, and is allowed to
    // modify itself.
    template<typename CallbackT,
    ROCKET_DISABLE_IF(::std::is_same<::std::decay_t<CallbackT>, Easy_Timer>::value)>
    explicit
    Easy_Timer(CallbackT&& cb)
      : m_cb_obj(new_sh<::std::decay_t<CallbackT>>(::std::forward<CallbackT>(cb))),
        m_cb_thunk(callback_thunk<::std::decay_t<CallbackT>>)
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_Timer);

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
