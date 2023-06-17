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
using ::asteria::generate_random_seed;

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
throw_runtime_error_with_backtrace(const char* file, long line, const char* func, string&& msg);

#define POSEIDON_THROW(TEMPLATE, ...)  \
    (::poseidon::throw_runtime_error_with_backtrace(__FILE__, __LINE__, __FUNCTION__,  \
       ::asteria::format_string(  \
         (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__)  \
       ),  \
     __builtin_unreachable())

// Converts all ASCII letters in a string into uppercase.
string
ascii_uppercase(string text);

// Converts all ASCII letters in a string into lowercase.
string
ascii_lowercase(string text);

// Removes all leading and trailing blank characters.
string
ascii_trim(string text);

// Checks whether two strings equal.
template<typename StringT, typename OtherT>
constexpr
bool
ascii_ci_equal(const StringT& text, const OtherT& other)
  {
    return ::rocket::ascii_ci_equal(text.c_str(), text.length(), other.c_str(), other.length());
  }

// Checks whether this list contains the specified token.
// Tokens are case-insensitive.
ROCKET_PURE
bool
ascii_ci_has_token(stringR text, char delim, const char* token, size_t len);

template<typename OtherT>
inline
bool
ascii_ci_has_token(stringR text, char delim, const OtherT& other)
  {
    return noadl::ascii_ci_has_token(text, delim, other.c_str(), other.length());
  }

ROCKET_PURE inline
bool
ascii_ci_has_token(stringR text, const char* token, size_t len)
  {
    return noadl::ascii_ci_has_token(text, ',', token, len);
  }

template<typename OtherT>
inline
bool
ascii_ci_has_token(stringR text, const OtherT& other)
  {
    return noadl::ascii_ci_has_token(text, other.c_str(), other.length());
  }

// Split a string into a vector of tokens, and vice versa.
using cow_vstrings = ::rocket::cow_vector<string>;

size_t
explode(cow_vstrings& segments, stringR text, char delim = ',', size_t limit = SIZE_MAX);

size_t
implode(string& text, const cow_vstrings& segments, char delim = ',');

}  // namespace poseidon
#endif
