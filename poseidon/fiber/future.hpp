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
  public:
    using value_type       = ValueT;
    using const_reference  = ::std::add_lvalue_reference_t<const ValueT>;
    using reference        = ::std::add_lvalue_reference_t<ValueT>;

  private:
    // These fields are initialized according to the future state.
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
        if(this->m_state.load() != future_state_value)
          this->do_abstract_future_check_value(typeid(ValueT).name(), this->m_exptr);

        // This cast is necessary when `const_reference` is void.
        return (const_reference) this->m_value[0];
      }

    // Gets the value if one has been set, or throws an exception otherwise.
    reference
    value()
      {
        if(this->m_state.load() != future_state_value)
          this->do_abstract_future_check_value(typeid(ValueT).name(), this->m_exptr);

        // This cast is necessary when `reference` is void.
        return (reference) this->m_value[0];
      }

    // Sets a value, or does nothing if another value or exception has been set.
    template<typename... ParamsT>
    void
    set_value(ParamsT&&... params)
      {
        plain_mutex::unique_lock lock(this->m_init_mutex);
        if(ROCKET_UNEXPECT(this->m_state.load() != future_state_empty))
          return;

        ::rocket::construct(this->m_value, ::std::forward<ParamsT>(params)...);
        this->m_state.store(future_state_value);
        this->do_abstract_future_signal_nolock();
      }

    // Sets an exception, or does nothing if another value or exception has been set.
    void
    set_exception(const ::std::exception_ptr& exptr_opt) noexcept
      {
        plain_mutex::unique_lock lock(this->m_init_mutex);
        if(ROCKET_UNEXPECT(this->m_state.load() != future_state_empty))
          return;

        ::rocket::construct(this->m_exptr, exptr_opt);
        this->m_state.store(future_state_exception);
        this->do_abstract_future_signal_nolock();
      }
  };

template<typename ValueT>
future<ValueT>::
~future()
  {
    switch(this->m_state.load()) {
      case future_state_empty:
        break;

      case future_state_value:
        ::rocket::destroy(this->m_value);
        break;

      case future_state_exception:
        ::rocket::destroy(this->m_exptr);
        break;
    }
  }

}  // namespace poseidon
#endif
