// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TIMER_
#define POSEIDON_EASY_EASY_TIMER_

#include "../fwd.hpp"
namespace poseidon {

class Easy_Timer
  {
  private:
    using thunk_function = void (void*, int64_t);
    thunk_function* m_cb_thunk;
    shared_ptr<void> m_cb_obj;

    shared_ptr<void> m_uniq;
    shared_ptr<Abstract_Timer> m_timer;

  public:
    // Constructs a timer. The argument shall be an invocable object taking
    // `(int64_t now)`, where `now` is the number of nanoseconds since system
    // startup. This timer stores a copy of the callback, which is invoked
    // accordingly in the main thread. The callback object is never copied,
    // and is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_DISABLE_IF(::std::is_same<::std::decay_t<CallbackT>, Easy_Timer>::value)>
    explicit
    Easy_Timer(CallbackT&& cb)
      {
        using cb_obj_type = ::std::decay_t<CallbackT>;
        this->m_cb_obj = ::std::make_shared<cb_obj_type>(::std::forward<CallbackT>(cb));

        this->m_cb_thunk = [](void* ptr, int64_t now)
          {
            // We need this as the type of the callback has been erased.
            ::std::invoke(*(cb_obj_type*) ptr, now);
          };
      }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_Timer);

    // Starts a timer if none is running, or resets the running one. The timer
    // callback will be called after `delay` nanoseconds, and then, if `period`
    // is non-zero, periodically every `period` nanoseconds. If `period` is
    // zero, the timer will only be called once.
    // If an exception is thrown, there is no effect.
    void
    start(int64_t delay, int64_t period);

    // Stops the timer, if one is running.
    void
    stop() noexcept;
  };

}  // namespace poseidon
#endif
