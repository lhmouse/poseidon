// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_FUTURE_
#define POSEIDON_FIBER_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
namespace poseidon {

template<typename ValueT>
class future
  : public Abstract_Future
  {
    using value_type       = ValueT;
    using const_reference  = ::std::add_lvalue_reference_t<const ValueT>;
    using reference        = ::std::add_lvalue_reference_t<ValueT>;

  private:
    static constexpr Future_State xstate_empty   = (Future_State) 0;  // future_state_empty
    static constexpr Future_State xstate_value   = (Future_State) 1;  // future_state_value
    static constexpr Future_State xstate_except  = (Future_State) 2;  // future_state_exception

    union {
      ::std::conditional_t<::std::is_void<ValueT>::value, int, ValueT> m_value[1];
      exception_ptr m_exptr[1];
    };

  public:
    // Constructs an empty future.
    explicit
    future() noexcept = default;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(future);

    // Gets the value if one has been set, or throws an exception otherwise.
    const_reference
    value() const
      {
        // If no value has been set, throw an exception.
        if(this->m_state.load() != xstate_value)
          this->do_abstract_future_check_value(typeid(ValueT).name(), this->m_exptr);

        // This cast is necessary when `const_reference` is void.
        return (const_reference) this->m_value[0];
      }

    // Gets the value if one has been set, or throws an exception otherwise.
    reference
    value()
      {
        // If no value has been set, throw an exception.
        if(this->m_state.load() != xstate_value)
          this->do_abstract_future_check_value(typeid(ValueT).name(), this->m_exptr);

        // This cast is necessary when `reference` is void.
        return (reference) this->m_value[0];
      }

    // Sets a value.
    template<typename... ParamsT>
    void
    set_value(ParamsT&&... params)
      {
        // If a value or exception has already been set, this function shall
        // do nothing.
        plain_mutex::unique_lock lock(this->m_init_mutex);
        if(this->m_state.load() != xstate_empty)
          return;

        // Construct the value.
        ::rocket::construct(this->m_value, ::std::forward<ParamsT>(params)...);
        this->m_state.store(xstate_value);
        this->do_abstract_future_signal_nolock();
      }

    // Sets an exception.
    void
    set_exception(const ::std::exception_ptr& exptr_opt) noexcept
      {
        // If a value or exception has already been set, this function shall
        // do nothing.
        plain_mutex::unique_lock lock(this->m_init_mutex);
        if(this->m_state.load() != xstate_empty)
          return;

        // Construct the exception pointer.
        ::rocket::construct(this->m_exptr, exptr_opt);
        this->m_state.store(xstate_except);
        this->do_abstract_future_signal_nolock();
      }
  };

template<typename ValueT>
future<ValueT>::
~future()
  {
    switch(this->m_state.load()) {
      case xstate_empty:
        break;

      case xstate_value:
        ::rocket::destroy(this->m_value);
        break;

      case xstate_except:
        ::rocket::destroy(this->m_exptr);
        break;
    }
  }

}  // namespace poseidon
#endif
