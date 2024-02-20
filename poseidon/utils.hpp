// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_UTILS_
#define POSEIDON_UTILS_

#include "fwd.hpp"
#include "static/async_logger.hpp"
namespace poseidon {

// Converts all ASCII letters in a string into uppercase.
cow_string
ascii_uppercase(cow_string text);

// Converts all ASCII letters in a string into lowercase.
cow_string
ascii_lowercase(cow_string text);

// Removes all leading and trailing blank characters.
cow_string
ascii_trim(cow_string text);

// Checks whether two strings compare equal.
constexpr
bool
ascii_ci_equal(cow_stringR str, cow_stringR cmp) noexcept
  {
    return ::rocket::ascii_ci_equal(str.data(), str.size(), cmp.data(), cmp.size());
  }

constexpr
bool
ascii_ci_equal(cow_stringR str, const char* sp) noexcept
  {
    return ::rocket::ascii_ci_equal(str.data(), str.size(), sp, ::rocket::xstrlen(sp));
  }

// Splits a string into a vector of tokens, and vice versa.
void
explode(vector<cow_string>& segments, cow_stringR text, char delim = ',', size_t limit = SIZE_MAX);

vector<cow_string>
explode(cow_stringR text, char delim = ',', size_t limit = SIZE_MAX);

void
implode(cow_string& text, const cow_string* segment_ptr, size_t segment_count, char delim = ',');

cow_string
implode(const cow_string* segment_ptr, size_t segment_count, char delim = ',');

void
implode(cow_string& text, const vector<cow_string>& segments, char delim = ',');

cow_string
implode(const vector<cow_string>& segments, char delim = ',');

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
      return out.swap(in);

    // Copy bytes from `in`, then clear it.
    out.putn(in.data(), in.size());
    in.clear();
    return out;
  }

// Converts 16 bytes into a hexadecimal string. Exactly 33 characters will be
// written, including a null terminator.
void
hex_encode_16_partial(char* str, const void* data) noexcept;

// Generates a cryptographically secure random integer in [0,UINT32_MAX]. Please
// be advised that this function may be very slow.
uint32_t
random_uint32() noexcept;

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

// Masks a string so it will not be visible as plaintext inside a debugger or a
// core dump file. This is primarily used to protect passwords.
void
mask_string(char* data, size_t size, uint32_t* next_mask_key_opt, uint32_t mask_key) noexcept;

// These are internal functions.
bool
enqueue_log_message(vfptr<cow_string&, void*> composer_thunk, void* composer,
                    uint8_t level, const char* func, const char* file, uint32_t line) noexcept;

::std::runtime_error
create_runtime_error(vfptr<cow_string&, void*> composer_thunk, void* composer,
                     const char* func, const char* file, uint32_t line);

// Compose a log message and enqueue it into the global logger. The `TEMPLATE`
// argument shall be a list of string literals in parentheses. Multiple strings
// are joined with line separators. `format()` is to be found via ADL.
#define POSEIDON_LOG_(LEVEL, TEMPLATE, ...)  \
    (::poseidon::async_logger.enabled(LEVEL)  \
     && __extension__  \
       ({  \
         auto IuChah0u = [&](::asteria::cow_string& quu1Opae)  \
           { format(quu1Opae, (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__);  };  \
         auto ohng0Ohh = [](::asteria::cow_string& iughih5B, void* fi8OhNgo)  \
           { (*static_cast<decltype(IuChah0u)*>(fi8OhNgo)) (iughih5B);  };  \
         ::poseidon::enqueue_log_message(+ohng0Ohh, &IuChah0u, LEVEL,  \
             __func__, __FILE__, __LINE__);  \
       }))

#define POSEIDON_LOG_FATAL(...)   POSEIDON_LOG_(0U, __VA_ARGS__)
#define POSEIDON_LOG_ERROR(...)   POSEIDON_LOG_(1U, __VA_ARGS__)
#define POSEIDON_LOG_WARN(...)    POSEIDON_LOG_(2U, __VA_ARGS__)
#define POSEIDON_LOG_INFO(...)    POSEIDON_LOG_(3U, __VA_ARGS__)
#define POSEIDON_LOG_DEBUG(...)   POSEIDON_LOG_(4U, __VA_ARGS__)
#define POSEIDON_LOG_TRACE(...)   POSEIDON_LOG_(5U, __VA_ARGS__)

// Throws an `std::runtime_error` object. The `TEMPLATE` argument shall be a
// list of string literals in parentheses. Multiple strings are joined with
// line separators. `format()` is to be found via ADL.
#define POSEIDON_THROW(TEMPLATE, ...)  \
    throw (__extension__  \
       ({  \
         auto IuChah0u = [&](::asteria::cow_string& quu1Opae)  \
           { format(quu1Opae, (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__);  };  \
         auto ohng0Ohh = [](::asteria::cow_string& iughih5B, void* fi8OhNgo)  \
           { (*static_cast<decltype(IuChah0u)*>(fi8OhNgo)) (iughih5B);  };  \
         ::poseidon::create_runtime_error(+ohng0Ohh, &IuChah0u,  \
             __func__, __FILE__, __LINE__);  \
       }))

}  // namespace poseidon
#endif
