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
    using function_type = void (xArgs...);
    POSEIDON_USING is_viable = ::std::is_invocable<Ts..., xArgs&&...>;

  private:
    vfptr<void*, xArgs&&...> m_func;
    shptr<void> m_obj;

  public:
    // Points this callback to a target object, with its type erased.
    template<typename xReal,
    ROCKET_ENABLE_IF(is_viable<xReal>::value)>
    explicit thunk(shptrR<xReal> obj) noexcept
      {
        this->m_func = [](void* p, xArgs&&... args) { (*(xReal*) p) (forward<xArgs>(args)...);  };
        this->m_obj = obj;
      }

    // And this is an optimized overload if the target object is a plain
    // function pointer, which can be stored into `m_obj` directly.
    explicit thunk(function_type* fptr) noexcept
      {
        this->m_func = nullptr;
        this->m_obj = shptr<void>(shptr<int>(), (void*)(intptr_t) fptr);
      }

    // Performs a virtual call to the target object.
    void
    operator()(xArgs... args) const
      {
        if(this->m_func)
          this->m_func(this->m_obj.get(), forward<xArgs>(args)...);
        else
          ((function_type*)(intptr_t) this->m_obj.get()) (forward<xArgs>(args)...);
      }
  };

}  // namespace poseidon
#endif
