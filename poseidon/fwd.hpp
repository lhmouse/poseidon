// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FWD_
#define POSEIDON_FWD_

#include "version.h"
#include <asteria/utils.hpp>
#include <rocket/ascii_case.hpp>
#include <rocket/atomic.hpp>
#include <rocket/mutex.hpp>
#include <rocket/recursive_mutex.hpp>
#include <rocket/condition_variable.hpp>
#include <rocket/linear_buffer.hpp>
#include <rocket/tinyfmt.hpp>
#include <rocket/unique_posix_fd.hpp>
#include <rocket/unique_posix_file.hpp>
#include <rocket/unique_posix_dir.hpp>
#include <rocket/xstring.hpp>
#include <array>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
namespace poseidon {
namespace noadl = poseidon;

// Aliases
using ::std::initializer_list;
using ::std::nullptr_t;
using ::std::max_align_t;
using ::std::int8_t;
using ::std::uint8_t;
using ::std::int16_t;
using ::std::uint16_t;
using ::std::int32_t;
using ::std::uint32_t;
using ::std::int64_t;
using ::std::uint64_t;
using ::std::intptr_t;
using ::std::uintptr_t;
using ::std::intmax_t;
using ::std::uintmax_t;
using ::std::ptrdiff_t;
using ::std::size_t;
using ::std::wint_t;
using ::std::exception;
using ::std::exception_ptr;
using ::std::type_info;
using ::std::pair;
template<typename T> using uniptr = ::std::unique_ptr<T>;  // default deleter
template<typename T> using shptr = ::std::shared_ptr<T>;
template<typename T> using wkptr = ::std::weak_ptr<T>;
using ::std::array;
using ::std::vector;
using ::std::deque;
using ::std::unordered_map;
using ::std::unordered_set;
using ::std::chrono::duration;
using ::std::chrono::nanoseconds;
using ::std::chrono::milliseconds;
using ::std::chrono::seconds;
using ::std::chrono::minutes;
using ::std::chrono::hours;
using days = ::std::chrono::duration<int64_t, ::std::ratio<86400>>;
using weeks = ::std::chrono::duration<int64_t, ::std::ratio<604800>>;
using ::std::chrono::time_point;
using ::std::chrono::system_clock;
using system_time = system_clock::time_point;
using unix_time = time_point<system_clock, seconds>;
using ::std::chrono::steady_clock;
using steady_time = steady_clock::time_point;

constexpr weeks zero_duration = { };
using ::std::static_pointer_cast;
using ::std::dynamic_pointer_cast;
using ::std::const_pointer_cast;
using ::std::chrono::duration_cast;
using ::std::chrono::time_point_cast;

constexpr
system_time
system_time_from_timespec(const ::timespec& ts) noexcept
  {
    return (system_time)(milliseconds) (ts.tv_sec * 1000LL + (uint32_t) ts.tv_nsec / 1000000U);
  }

using ::rocket::atomic;
using ::rocket::atomic_relaxed;
using ::rocket::atomic_acq_rel;
using ::rocket::atomic_seq_cst;
using plain_mutex = ::rocket::mutex;
using ::rocket::recursive_mutex;
using ::rocket::condition_variable;
using ::rocket::cow_vector;
using ::rocket::cow_hashmap;
using ::rocket::static_vector;
using string = ::rocket::cow_string;
using ::rocket::cow_u16string;
using ::rocket::cow_u32string;
using ::rocket::linear_buffer;
using ::rocket::tinybuf;
using ::rocket::tinyfmt;
using ::rocket::unique_posix_fd;
using ::rocket::unique_posix_file;
using ::rocket::unique_posix_dir;

using ::rocket::begin;
using ::rocket::end;
using ::rocket::swap;
using ::rocket::xswap;
using ::rocket::size;
using ::rocket::sref;
using ::rocket::nullopt;
using ::rocket::xstrlen;
using ::rocket::xstrchr;
using ::rocket::xstrcmp;
using ::rocket::xstrpcpy;
using ::rocket::xstrrpcpy;
using ::rocket::xmemchr;
using ::rocket::xmemcmp;
using ::rocket::xmempset;
using ::rocket::xmemrpset;
using ::rocket::xmempcpy;
using ::rocket::xmemrpcpy;

using ::rocket::nullopt_t;
using ::rocket::optional;
using ::rocket::variant;
using phsh_string = ::rocket::prehashed_string;

using stringR = const string&;
using phsh_stringR = const phsh_string&;
template<typename T> using shptrR = const shptr<T>&;
template<typename T> using wkptrR = const wkptr<T>&;

// Enumerations
enum zlib_Format : uint8_t
  {
    zlib_format_deflate  = 0,
    zlib_format_raw      = 1,
    zlib_format_gzip     = 2,
  };

enum Async_State : uint8_t
  {
    async_state_pending    = 0,
    async_state_suspended  = 1,
    async_state_running    = 2,
    async_state_finished   = 3,
  };

enum IP_Address_Class : uint8_t
  {
    ip_address_class_unspecified  = 0,  // all zeroes
    ip_address_class_reserved     = 1,
    ip_address_class_public       = 2,
    ip_address_class_loopback     = 3,
    ip_address_class_private      = 4,
    ip_address_class_link_local   = 5,
    ip_address_class_multicast    = 6,
    ip_address_class_broadcast    = 7,  // IPv4 only
  };

enum Socket_State : uint8_t
  {
    socket_state_pending      = 0,
    socket_state_established  = 1,
    socket_state_closing      = 2,
    socket_state_closed       = 3,
  };

enum Connection_Event : uint8_t
  {
    connection_event_null    = 0,
    connection_event_open    = 1,
    connection_event_stream  = 2,
    connection_event_closed  = 3,
  };

// Base types
class Config_File;
class charbuf_256;
class Abstract_Timer;
class Abstract_Async_Task;
class Deflator;
class Inflator;

// Fiber types
class Abstract_Future;
class DNS_Future;
class Read_File_Future;
class Abstract_Fiber;

// Socket types
class Socket_Address;
class Abstract_Socket;
class Listen_Socket;
class UDP_Socket;
class TCP_Socket;
class SSL_Socket;

// HTTP types
class HTTP_DateTime;
class HTTP_Value;
class HTTP_Header;

// Easy types
// Being 'easy' means all callbacks are invoked in fibers and can perform
// async/await operations. These are suitable for agile development.
class Easy_Timer;
class Easy_UDP_Server;
class Easy_UDP_Client;
class Easy_TCP_Server;
class Easy_TCP_Client;
class Easy_SSL_Server;
class Easy_SSL_Client;
class Easy_Deflator;
class Easy_Inflator;

// Singletons
extern atomic_relaxed<int> exit_signal;
extern class Main_Config& main_config;
extern class Fiber_Scheduler& fiber_scheduler;

extern class Async_Logger& async_logger;
extern class Timer_Driver& timer_driver;
extern class Async_Task_Executor& async_task_executor;
extern class Network_Driver& network_driver;

// General utilities
template<typename CopyT, typename... ArgsT>
static
void
callback_thunk(void* ptr, ArgsT... args)
  {
    ::std::invoke(*(CopyT*) ptr, ::std::forward<ArgsT>(args)...);
  }

template<typename... ArgsT>
using callback_thunk_ptr = void (*)(void*, ArgsT...);

template<typename ValueT, typename... ArgsT>
ROCKET_ALWAYS_INLINE
uniptr<ValueT>
new_uni(ArgsT&&... args)
  {
    return ::std::make_unique<ValueT>(::std::forward<ArgsT>(args)...);
  }

template<typename ValueT, typename... ArgsT>
ROCKET_ALWAYS_INLINE
shptr<ValueT>
new_sh(ArgsT&&... args)
  {
    return ::std::make_shared<ValueT>(::std::forward<ArgsT>(args)...);
  }

// Composes a string and submits it to the logger.
enum Log_Level : uint8_t
  {
    log_level_fatal  = 0,
    log_level_error  = 1,
    log_level_warn   = 2,
    log_level_info   = 3,
    log_level_debug  = 4,
    log_level_trace  = 5,
  };

struct Log_Context
  {
    const char* file;
    uint32_t line;
    Log_Level level;
    const char* func;
  };

ROCKET_CONST
bool
do_async_logger_check_level(Log_Level level) noexcept;

void
do_async_logger_enqueue(const Log_Context& ctx, void* cb_obj, callback_thunk_ptr<string&> cb_thunk) noexcept;

template<typename... ParamsT>
inline
bool
do_async_logger_enqueue_generic(const Log_Context& ctx, const ParamsT&... params) noexcept
  {
    auto my_thunk = [&](string& msg) { ::asteria::format(msg, params...);  };
    do_async_logger_enqueue(ctx, ::std::addressof(my_thunk), callback_thunk<decltype(my_thunk)>);
    return true;
  }

// Define helper macros that compose log messages.
// The `TEMPLATE` argument shall be a list of string literals in parentheses.
// Multiple strings are joined with new line characters. No trailing new line
// is appended. An example is:
//  `POSEIDON_LOG_ERROR(("invalid argument: $1", "error: $2"), arg, err);`
#define POSEIDON_LOG_GENERIC(LEVEL, TEMPLATE, ...)  \
  (::poseidon::do_async_logger_check_level(::poseidon::log_level_##LEVEL)  \
   && ::poseidon::do_async_logger_enqueue_generic(  \
          { __FILE__, __LINE__, ::poseidon::log_level_##LEVEL, __FUNCTION__ }, \
          (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__))

#define POSEIDON_LOG_FATAL(...)   POSEIDON_LOG_GENERIC(fatal, __VA_ARGS__)
#define POSEIDON_LOG_ERROR(...)   POSEIDON_LOG_GENERIC(error, __VA_ARGS__)
#define POSEIDON_LOG_WARN(...)    POSEIDON_LOG_GENERIC(warn,  __VA_ARGS__)
#define POSEIDON_LOG_INFO(...)    POSEIDON_LOG_GENERIC(info,  __VA_ARGS__)
#define POSEIDON_LOG_DEBUG(...)   POSEIDON_LOG_GENERIC(debug, __VA_ARGS__)
#define POSEIDON_LOG_TRACE(...)   POSEIDON_LOG_GENERIC(trace, __VA_ARGS__)

}  // namespace poseidon
#endif
