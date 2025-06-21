// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_DETAILS_ERROR_HANDLING_
#define POSEIDON_DETAILS_ERROR_HANDLING_

#include "../fwd.hpp"
namespace poseidon {

using message_composer_fn = void (tinyfmt&, const void*);

bool
do_is_log_enabled(uint8_t level) noexcept __attribute__((__pure__));

bool
do_push_log_message(uint8_t level, const char* func, const char* file, uint32_t line,
                    const void* composer, message_composer_fn* composer_fn);

::std::runtime_error
do_create_runtime_error(const char* func, const char* file, uint32_t line,
                        const void* composer, message_composer_fn* composer_fn);

}  // namespace poseidon
#endif
