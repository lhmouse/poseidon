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
    shptr<void> m_obj;
    vfptr<void*, xArgs&&...> m_func;
    void* m_padding;

  public:
    // Points this thunk to `obj`, with its type erased.
    template<typename xReal,
    ROCKET_ENABLE_IF(is_viable<xReal>::value),
    ROCKET_DISABLE_IF(::std::is_constructible<thunk, xReal&&>::value)>
    explicit thunk(xReal&& obj)
      : m_obj(new_sh<typename ::std::decay<xReal>::type>(forward<xReal>(obj))),
        m_func([](void* p, xArgs&&... args) -> void
          { ::std::invoke(static_cast<typename ::std::decay<xReal>::type*>(p),
                          forward<xArgs>(args)...);  })
      { }

    // Stores a pointer to `fptr` directly, without allocating memory.
    explicit thunk(void fptr(xArgs...)) noexcept
      : m_obj(shptr<int>(), reinterpret_cast<void*>(fptr)),
        m_func(nullptr)
      { }

    // Performs a virtual call to the target object.
    void
    operator()(xArgs... args) const
      {
        if(this->m_func)
          this->m_func(this->m_obj.get(), forward<xArgs>(args)...);
        else
          reinterpret_cast<void (*)(xArgs...)>(this->m_obj.get()) (
                                                 forward<xArgs>(args)...);
      }
  };

}  // namespace poseidon
#endif
