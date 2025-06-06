// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_UTILS_
#define POSEIDON_UTILS_

#include "fwd.hpp"
#include "static/logger.hpp"
namespace poseidon {

// Splits a string into a vector of tokens, and vice versa.
void
explode(cow_vector<cow_string>& segments, const cow_string& text, char delim = ',', size_t limit = SIZE_MAX);

cow_vector<cow_string>
explode(const cow_string& text, char delim = ',', size_t limit = SIZE_MAX);

void
implode(cow_string& text, const cow_string* segment_ptr, size_t segment_count, char delim = ',');

cow_string
implode(const cow_string* segment_ptr, size_t segment_count, char delim = ',');

void
implode(cow_string& text, const cow_vector<cow_string>& segments, char delim = ',');

cow_string
implode(const cow_vector<cow_string>& segments, char delim = ',');

// Prints a quoted UTF-8 string, like in JSON.
void
quote_json_string(tinybuf& buf, const cow_string& str);

void
quote_json_string(tinyfmt& fmt, const cow_string& str);

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

// Generates a cryptographically secure random byte sequence. Please be advised
// that this function may be very slow.
void
random_bytes(void* ptr, size_t size) noexcept;

ROCKET_ALWAYS_INLINE
uint32_t
random_uint32() noexcept
  {
    uint32_t bits;
    random_bytes(&bits, sizeof(bits));
    return bits;
  }

ROCKET_ALWAYS_INLINE
uint64_t
random_uint64() noexcept
  {
    uint64_t bits;
    random_bytes(&bits, sizeof(bits));
    return bits;
  }

// Performs conversion between `timespec` and `system_time`.
ROCKET_ALWAYS_INLINE
system_time
system_time_from_timespec(const struct ::timespec& ts) noexcept
  {
    return system_clock::from_time_t(ts.tv_sec) + nanoseconds(ts.tv_nsec);
  }

ROCKET_ALWAYS_INLINE
void
timespec_from_system_time(struct ::timespec& ts, system_time tm) noexcept
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
parse_network_reference(Network_Reference& caddr, chars_view str) noexcept;

// These are internal functions.
bool
enqueue_log_message(void composer_callback(cow_string&, void*), void* composer,
                    uint8_t level, const char* func, const char* file,
                    uint32_t line) noexcept;

::std::runtime_error
create_runtime_error(void composer_callback(cow_string&, void*), void* composer,
                     const char* func, const char* file, uint32_t line);

// Compose a log message and enqueue it into the global logger. The `TEMPLATE`
// argument shall be a list of string literals in parentheses. Multiple strings
// are joined with line separators. `format()` is to be found via ADL.
#define POSEIDON_LOG_(LEVEL, TEMPLATE, ...)  \
    (::poseidon::logger.enabled(LEVEL)  \
     && __extension__  \
      ({  \
        using ::asteria::format;  \
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
        using ::asteria::format;  \
        auto IuChah0u = [&](::asteria::cow_string& quu1Opae)  \
          { format(quu1Opae, (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__);  };  \
        auto ohng0Ohh = [](::asteria::cow_string& iughih5B, void* fi8OhNgo)  \
          { (*static_cast<decltype(IuChah0u)*>(fi8OhNgo)) (iughih5B);  };  \
        ::poseidon::create_runtime_error(+ohng0Ohh, &IuChah0u,  \
            __func__, __FILE__, __LINE__);  \
      }))

#define POSEIDON_CHECK(...)  \
    (bool(__VA_ARGS__) ? (void) 0 :  \
      throw (__extension__  \
        ({  \
          using ::asteria::format;  \
          auto Vai4feeP = [&](::asteria::cow_string& Pha2Ae5i)  \
            { Pha2Ae5i = &"POSEIDON_CHECK failed: " #__VA_ARGS__;  };  \
          auto iez4eeY9 = [](::asteria::cow_string& hohv8Ing, void* fi8OhNgo)  \
            { (*static_cast<decltype(Vai4feeP)*>(fi8OhNgo)) (hohv8Ing);  };  \
          ::poseidon::create_runtime_error(+iez4eeY9, &Vai4feeP,  \
              __func__, __FILE__, __LINE__);  \
        })))

#define POSEIDON_CATCH_EVERYTHING(...)  \
    __extension__  \
      ({  \
        try { (void) (__VA_ARGS__);  }  \
        catch(::std::exception& af9cohMo)  \
          { POSEIDON_LOG_ERROR(("Ignoring exception: $1"), af9cohMo);  }  \
        (void) 0;  \
      })

}  // namespace poseidon
#endif
