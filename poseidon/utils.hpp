// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_UTILS_
#define POSEIDON_UTILS_

#include "fwd.hpp"
#include <asteria/utils.hpp>
#include <cstdio>
namespace poseidon {

// Performs a syscall and retries upon interrupts.
// Note the arguments may be evaluated more than once.
#define POSEIDON_SYSCALL_LOOP(...)  \
    __extension__ ({  \
      auto wdLAlUiJ = (__VA_ARGS__);  \
      while(ROCKET_UNEXPECT(wdLAlUiJ < 0) && (errno == EINTR))  \
        /* retry */ wdLAlUiJ = (__VA_ARGS__);  \
      wdLAlUiJ;  \
    })

// Throws an exception, with backtraces.
[[noreturn]]
void
backtrace_and_throw(const char* file, long line, const char* func, cow_string&& msg);

#define POSEIDON_THROW(TEMPLATE, ...)  \
    __extension__ ({  \
      ::rocket::cow_string iegh0Ohy;  \
      format(iegh0Ohy, (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__);  \
      ::poseidon::backtrace_and_throw(__FILE__, __LINE__, __func__,  \
           static_cast<::rocket::cow_string&&>(iegh0Ohy));  \
      __builtin_unreachable();  \
    })

// Splices two buffers. After this function returns, `in` will be empty.
inline
linear_buffer&
splice_buffers(linear_buffer& out, linear_buffer&& in)
  {
    if(&out == &in)
      return out;

    if(in.empty())
      return out;

    // Don't bother making a copy if `out` is empty.
    if(ROCKET_EXPECT(out.empty()))
      out.swap(in);
    else {
      out.putn(in.data(), in.size());
      in.clear();
    }
    return out;
  }

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
  { return ::rocket::ascii_ci_equal(text.c_str(), text.length(), other.c_str(), other.length());  }

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

// This resembles a URI without the `scheme://` and `userinfo@` parts.
struct Network_Reference
  {
    chars_view host;
    chars_view port;
    chars_view path;
    chars_view query;
    chars_view fragment;
    uint16_t port_num = 0;
    bool is_ipv6 = false;
  };

// Parses a network reference. `str` shall start with a host name, followed by an
// optional port, an optional absolute path, an optional query string, and an
// optional fragment. If any optional part is absent, the corresponding field in
// `caddr` is left unmodified. They may be initialized with default values before
// calling this function.
// Returns the number of character that have been parsed. Zero is returned if the
// address string is malformed.
size_t
parse_network_reference(Network_Reference& caddr, chars_view str) noexcept;

}  // namespace poseidon
#endif
