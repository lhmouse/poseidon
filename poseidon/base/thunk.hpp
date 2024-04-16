// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_THUNK_
#define POSEIDON_BASE_THUNK_

#include "../fwd.hpp"
namespace poseidon {

template<typename... xArgs>
class thunk
  {
  public:
    template<typename xReal>
    using is_viable = ::std::is_invocable<
                typename ::std::decay<xReal>::type&, xArgs&&...>;

  private:
    ::rocket::refcnt_ptr<::asteria::Rcbase> m_obj;
    union {
      vfptr<xArgs...> m_pfn;
      vfptr<::asteria::Rcbase*, xArgs&&...> m_ofn;
    };

  public:
    constexpr thunk() noexcept
      : m_obj(), m_pfn()
      { }

    explicit constexpr thunk(void fptr(xArgs...)) noexcept
      : m_obj(), m_pfn(fptr)
      { }

    thunk(const thunk& other) noexcept
      : m_obj(other.m_obj), m_pfn(other.m_pfn)
      { }

    thunk(thunk&& other) noexcept
      : m_obj(move(other.m_obj)), m_pfn(exchange(other.m_pfn))
      { }

    explicit constexpr operator bool() const noexcept
      { return this->m_pfn != nullptr;  }

    template<typename xReal,
    ROCKET_ENABLE_IF(is_viable<xReal>::value),
    ROCKET_DISABLE_SELF(thunk, xReal)>
    explicit thunk(xReal&& obj)
      {
        struct Container : ::asteria::Rcbase
          {
            typename ::std::decay<xReal>::type ofn;

            Container(xReal&& xobj) : ofn(forward<xReal>(xobj))  { }
            Container(const Container&) = delete;
            Container& operator=(const Container&) & = delete;
          };

        this->m_obj.reset(new Container(forward<xReal>(obj)));
        this->m_ofn = [](::asteria::Rcbase* b, xArgs&&... args)
          { static_cast<Container*>(b)->ofn(forward<xArgs>(args)...);  };
      }

    thunk&
    operator=(const thunk& other) & noexcept
      {
        this->m_obj = other.m_obj;
        this->m_pfn = other.m_pfn;
        return *this;
      }

    thunk&
    operator=(thunk&& other) & noexcept
      {
        this->m_obj = move(other.m_obj);
        this->m_pfn = exchange(other.m_pfn);
        return *this;
      }

    void
    operator()(xArgs... args) const
      {
        if(!this->m_pfn)
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                     "thunk: Attempt to call a null function object");

        if(!this->m_obj)
          this->m_pfn(forward<xArgs>(args)...);
        else
          this->m_ofn(this->m_obj.get(), forward<xArgs>(args)...);
      }
  };

}  // namespace poseidon
#endif
