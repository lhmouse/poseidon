// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_ASAN_FWD_
#define POSEIDON_THIRD_ASAN_FWD_

#include "../fwd.hpp"
#include <ucontext.h>
extern "C" void __sanitizer_start_switch_fiber(void**, const void*, size_t) noexcept;
extern "C" void __sanitizer_finish_switch_fiber(void*, const void**, size_t*) noexcept;
namespace poseidon {

ROCKET_ALWAYS_INLINE
void
asan_fiber_switch_start(void*& save, const ::stack_t& st) noexcept
  {
#ifdef POSEIDON_ENABLE_ADDRESS_SANITIZER
    __sanitizer_start_switch_fiber(&save, st.ss_sp, st.ss_size);
#else
    save = st.ss_sp;
#endif
  }

ROCKET_ALWAYS_INLINE
void
asan_fiber_switch_finish(::stack_t& st, void* save) noexcept
  {
#ifdef POSEIDON_ENABLE_ADDRESS_SANITIZER
    __sanitizer_finish_switch_fiber(save, (const void**) &(st.ss_sp), &(st.ss_size));
#else
    (void)! (st.ss_sp == save);
#endif
  }

}  // namespace poseidon
#endif
