// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_UTILS_
#define POSEIDON_UTILS_

#include "fwd.hpp"
#include "details/error_handling.hpp"
namespace poseidon {

// Compose a log message and enqueue it into the global logger. The `TEMPLATE`
// argument shall be a list of string literals in parentheses. Multiple strings
// are joined with line separators. `format()` is to be found via ADL.
#define POSEIDON_LOG_(LEVEL, TEMPLATE, ...)  \
  (::poseidon::do_is_log_enabled(LEVEL)  \
   &&  \
   ([&](const char* func_ce7d) -> bool  \
      __attribute__((__nothrow__, __noinline__))  \
    {  \
      try {  \
        auto c_Ru6q = [&](::rocket::tinyfmt& fmt_Ko0i)  \
          {  \
            using ::asteria::format;  \
            format(fmt_Ko0i, (::asteria::make_string_template TEMPLATE),  \
                    ##__VA_ARGS__);  \
          };  \
        \
        ::poseidon::do_push_log_message(\
            LEVEL, func_ce7d, __FILE__, __LINE__,  \
            &c_Ru6q,  \
            [](::rocket::tinyfmt& fmt_Ko0i, void* p_5Gae)  \
              { (* static_cast<decltype(c_Ru6q)*>(p_5Gae)) (fmt_Ko0i);  });  \
      }  \
      catch(...) { }  \
      return true;  \
    } (__func__)))

#define POSEIDON_LOG_FATAL(...)   POSEIDON_LOG_(0, __VA_ARGS__)
#define POSEIDON_LOG_ERROR(...)   POSEIDON_LOG_(1, __VA_ARGS__)
#define POSEIDON_LOG_WARN(...)    POSEIDON_LOG_(2, __VA_ARGS__)
#define POSEIDON_LOG_INFO(...)    POSEIDON_LOG_(3, __VA_ARGS__)
#define POSEIDON_LOG_DEBUG(...)   POSEIDON_LOG_(4, __VA_ARGS__)
#define POSEIDON_LOG_TRACE(...)   POSEIDON_LOG_(5, __VA_ARGS__)

// Throws an `std::runtime_error` object. The `TEMPLATE` argument shall be a
// list of string literals in parentheses. Multiple strings are joined with
// line separators. `format()` is to be found via ADL.
#define POSEIDON_THROW(TEMPLATE, ...)  \
  (throw \
   ([&](const char* func_ce7d) -> ::std::runtime_error  \
      __attribute__((__noinline__))  \
    {  \
      auto c_Ru6q = [&](::rocket::tinyfmt& fmt_Ko0i)  \
        {  \
          using ::asteria::format;  \
          format(fmt_Ko0i, (::asteria::make_string_template TEMPLATE),  \
                  ##__VA_ARGS__);  \
        };  \
      \
      return ::poseidon::do_create_runtime_error(\
          func_ce7d, __FILE__, __LINE__,  \
          &c_Ru6q,  \
          [](::rocket::tinyfmt& fmt_Ko0i, void* p_5Gae)  \
            { (* static_cast<decltype(c_Ru6q)*>(p_5Gae)) (fmt_Ko0i);  });  \
    } (__func__)))

#define POSEIDON_CATCH_EVERYTHING(...)  \
  do  \
    try { static_cast<void>(__VA_ARGS__);  }  \
    catch(::std::exception& ex_si8R)  \
    { POSEIDON_LOG_WARN(("POSEIDON_CATCH_EVERYTHING: $1"), ex_si8R);  }  \
  while(false)  // no semicolon

#define POSEIDON_CHECK(...)  \
  (static_cast<bool>(__VA_ARGS__)  \
    ? void()  \
    : POSEIDON_THROW(("POSEIDON_CHECK: " #__VA_ARGS__)));

// Splits a string into a vector of tokens, and vice versa.
void
explode(cow_vector<cow_string>& segments, const cow_string& text, char delim = ',',
        size_t limit = SIZE_MAX);

cow_vector<cow_string>
explode(const cow_string& text, char delim = ',', size_t limit = SIZE_MAX);

void
implode(cow_string& text, const cow_string* segment_ptr, size_t segment_count,
        char delim = ',');

cow_string
implode(const cow_string* segment_ptr, size_t segment_count, char delim = ',');

void
implode(cow_string& text, const cow_vector<cow_string>& segments, char delim = ',');

cow_string
implode(const cow_vector<cow_string>& segments, char delim = ',');

// Prints a quoted UTF-8 string, like in JSON.
void
quote_json_string(tinyfmt& buf, const cow_string& str);

// Converts 16 bytes into a hexadecimal string. Exactly 33 characters will be
// written, including a null terminator.
void
hex_encode_16_partial(char* str, const void* data)
  noexcept;

// Generates a cryptographically secure random byte sequence. Please be advised
// that this function may be very slow.
void
random_bytes(void* ptr, size_t size)
  noexcept;

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

// Performs conversion between `timespec` and `system_time`.
ROCKET_ALWAYS_INLINE
system_time
system_time_from_timespec(const struct timespec& ts)
  noexcept
  {
    return system_clock::from_time_t(ts.tv_sec) + nanoseconds(ts.tv_nsec);
  }

ROCKET_ALWAYS_INLINE
void
timespec_from_system_time(struct timespec& ts, system_time tm)
  noexcept
  {
    int64_t ns = time_point_cast<nanoseconds>(tm).time_since_epoch().count();
    uint64_t shifted_ns = static_cast<uint64_t>(ns) + 9223372036000000000;
    ts.tv_sec = static_cast<::time_t>(shifted_ns / 1000000000) - 9223372036;
    ts.tv_nsec = static_cast<long>(shifted_ns % 1000000000);
  }

// This resembles a partial URI like `host[:port]/path?query#fragment`. The
// `scheme://` and `userinfo@` parts are not supported.
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
parse_network_reference(Network_Reference& caddr, chars_view str)
  noexcept;

// URL-decodes and then canonicalizes the path (of a network reference). The
// result path will always start with a slash. If the source path appears to
// denote a directory, the the result path will also end with a slash.
cow_string
decode_and_canonicalize_uri_path(chars_view path);

}  // namespace poseidon
#endif
