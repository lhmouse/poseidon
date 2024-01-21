// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_ASAN_FWD_
#define POSEIDON_THIRD_ASAN_FWD_

#include "../fwd.hpp"
#include <ucontext.h>
#ifdef POSEIDON_ENABLE_ADDRESS_SANITIZER
extern "C" void __sanitizer_start_switch_fiber(void**, const void*, size_t) noexcept;
extern "C" void __sanitizer_finish_switch_fiber(void*, const void**, size_t*) noexcept;
#endif  // POSEIDON_ENABLE_ADDRESS_SANITIZER
namespace poseidon {

ROCKET_ALWAYS_INLINE
void
asan_fiber_switch_start(void*& save, const ::ucontext_t* uctx) noexcept
  {
#ifdef POSEIDON_ENABLE_ADDRESS_SANITIZER
    __sanitizer_start_switch_fiber(&save, uctx->uc_stack.ss_sp, uctx->uc_stack.ss_size);
#else
    save = uctx->uc_stack.ss_sp;
#endif
  }

ROCKET_ALWAYS_INLINE
void
asan_fiber_switch_finish(void* save) noexcept
  {
#ifdef POSEIDON_ENABLE_ADDRESS_SANITIZER
    __sanitizer_finish_switch_fiber(save, nullptr, nullptr);
#else
    (void) save;
#endif
  }

}  // namespace poseidon
#endif
