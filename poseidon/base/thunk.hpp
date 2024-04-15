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
    using is_viable = ::std::is_invocable<typename ::std::decay<xReal>::type&,
                                          xArgs&&...>;

  private:
    ::rocket::refcnt_ptr<::asteria::Rcbase> m_obj;
    union {
      vfptr<xArgs...> m_pfn;
      vfptr<::asteria::Rcbase*, xArgs&&...> m_ofn;
    };

  public:
    // Stores a pointer to `fptr` directly, without allocating memory.
    explicit constexpr thunk(void fptr(xArgs...)) noexcept
      : m_obj(), m_pfn(fptr)
      { }

    // Points this thunk to a copy of `obj`, with its type erased.
    template<typename xReal,
    ROCKET_ENABLE_IF(is_viable<xReal>::value),
    ROCKET_DISABLE_IF(::std::is_base_of<thunk,
        typename ::std::remove_reference<xReal>::type>::value)>
    explicit thunk(xReal&& obj)
      {
        struct Container : ::asteria::Rcbase
          {
            typename ::std::decay<xReal>::type ofn;

            explicit Container(xReal&& xobj) : ofn(forward<xReal>(xobj))  { }
            Container(const Container&) = delete;
            Container& operator=(const Container&) & = delete;
          };

        this->m_obj.reset(new Container(forward<xReal>(obj)));
        this->m_ofn = [](::asteria::Rcbase* b, xArgs&&... args)
            { static_cast<Container*>(b)->ofn(forward<xArgs>(args)...);  };
      }

    // Performs a virtual call to the target object.
    void
    operator()(xArgs... args) const
      {
        if(!this->m_obj)
          this->m_pfn(forward<xArgs>(args)...);
        else
          this->m_ofn(this->m_obj.get(), forward<xArgs>(args)...);
      }
  };

}  // namespace poseidon
#endif
