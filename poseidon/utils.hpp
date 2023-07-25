// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_UTILS_
#define POSEIDON_UTILS_

#include "fwd.hpp"
#include <asteria/utils.hpp>
#include <cstdio>
namespace poseidon {

using ::rocket::min;
using ::rocket::max;
using ::rocket::clamp;
using ::rocket::clamp_cast;
using ::rocket::is_any_of;
using ::rocket::is_none_of;
using ::rocket::all_of;
using ::rocket::any_of;
using ::rocket::none_of;
using ::asteria::format;
using ::asteria::format_string;
using ::asteria::weaken_enum;

// Performs a syscall and retries upon interrupts.
// Note the arguments may be evaluated more than once.
#define POSEIDON_SYSCALL_LOOP(...)  \
  ([&]{  \
    for(;;) {  \
      auto wdLAlUiJ = (__VA_ARGS__);  \
      if((wdLAlUiJ >= 0) || (errno != EINTR))  \
        return wdLAlUiJ;  \
    }  \
  }())

// Throws an exception, with backtraces.
[[noreturn]]
void
throw_runtime_error_with_backtrace(const char* file, long line, const char* func, cow_string&& msg);

#define POSEIDON_THROW(TEMPLATE, ...)  \
    (::poseidon::throw_runtime_error_with_backtrace(__FILE__, __LINE__, __FUNCTION__,  \
       ::asteria::format_string(  \
         (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__)  \
       ),  \
     __builtin_unreachable())

// Converts all ASCII letters in a string into uppercase.
cow_string
ascii_uppercase(cow_string text);

// Converts all ASCII letters in a string into lowercase.
cow_string
ascii_lowercase(cow_string text);

// Removes all leading and trailing blank characters.
cow_string
ascii_trim(cow_string text);

// Checks whether two strings equal.
template<typename StringT, typename OtherT>
constexpr
bool
ascii_ci_equal(const StringT& text, const OtherT& other)
  {
    return ::rocket::ascii_ci_equal(text.c_str(), text.length(), other.c_str(), other.length());
  }

// Splits a string into a vector of tokens, and vice versa.
size_t
explode(cow_vector<cow_string>& segments, cow_stringR text, char delim = ',', size_t limit = SIZE_MAX);

size_t
implode(cow_string& text, const cow_vector<cow_string>& segments, char delim = ',');

// Converts 16 bytes into a hexadecimal string. Exactly 33 characters will be
// written. A null terminator will always be appended.
char*
hex_encode_16_partial(char* str, const void* data) noexcept;

// Generates a cryptographically secure random integer in [0,UINT32_MAX].
uint32_t
random_uint32() noexcept;

// Generates a cryptographically secure random integer in [0,UINT64_MAX].
uint64_t
random_uint64() noexcept;

// Generates a cryptographically secure random integer in [0,1).
float
random_float() noexcept;

// Generates a cryptographically secure random integer in [0,1).
double
random_double() noexcept;

}  // namespace poseidon
#endif
