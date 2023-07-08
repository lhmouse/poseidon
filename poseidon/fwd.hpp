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
#include <rocket/tinyfmt_str.hpp>
#include <rocket/tinyfmt_ln.hpp>
#include <rocket/unique_posix_fd.hpp>
#include <rocket/unique_posix_file.hpp>
#include <rocket/unique_posix_dir.hpp>
#include <rocket/xstring.hpp>
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
namespace poseidon {
namespace noadl = poseidon;

// Macros
#define POSEIDON_HIDDEN_X_STRUCT(C, S)  \
  struct __attribute__((__visibility__("hidden"))) C::X_##S : S  \
    {  \
      using S::S;  \
      using S::operator=;  \
    }  // no semicolon

#define POSEIDON_VISIBILITY_HIDDEN   \
  __attribute__((__visibility__("hidden"))) inline

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
using ::std::array;
using ::std::vector;
using ::std::deque;
using ::std::list;
using ::std::map;
using ::std::multimap;
using ::std::set;
using ::std::multiset;
using ::std::unordered_map;
using ::std::unordered_multimap;
using ::std::unordered_set;
using ::std::unordered_multiset;

using ::std::chrono::duration;
using ::std::chrono::nanoseconds;
using ::std::chrono::milliseconds;
using ::std::chrono::seconds;
using ::std::chrono::minutes;
using ::std::chrono::hours;
using days = ::std::chrono::duration<int64_t, ::std::ratio<86400>>;
using weeks = ::std::chrono::duration<int64_t, ::std::ratio<604800>>;
constexpr weeks zero_duration = { };

using ::std::chrono::time_point;
using ::std::chrono::system_clock;
using system_time = system_clock::time_point;
using unix_time = time_point<system_clock, seconds>;
using ::std::chrono::steady_clock;
using steady_time = steady_clock::time_point;

using ::std::static_pointer_cast;
using ::std::dynamic_pointer_cast;
using ::std::const_pointer_cast;
using ::std::chrono::duration_cast;
using ::std::chrono::time_point_cast;

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
using ::rocket::cow_string;
using ::rocket::cow_u16string;
using ::rocket::cow_u32string;
using ::rocket::linear_buffer;
using ::rocket::tinybuf;
using ::rocket::tinybuf_str;
using ::rocket::tinybuf_ln;
using ::rocket::tinyfmt;
using ::rocket::tinyfmt_str;
using ::rocket::tinyfmt_ln;
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
using ::rocket::xmemeq;
using ::rocket::xmempset;
using ::rocket::xmemrpset;
using ::rocket::xmempcpy;
using ::rocket::xmemrpcpy;

using ::rocket::nullopt_t;
using ::rocket::optional;
using ::rocket::variant;
using phsh_string = ::rocket::prehashed_string;

template<typename T, typename U> using cow_bivector = cow_vector<pair<T, U>>;
template<typename T> using uniptr = ::std::unique_ptr<T>;  // default deleter
template<typename T> using shptr = ::std::shared_ptr<T>;
template<typename T> using wkptr = ::std::weak_ptr<T>;

using cow_stringR = const cow_string&;
using phsh_stringR = const phsh_string&;
template<typename T> using shptrR = const shptr<T>&;
template<typename T> using wkptrR = const wkptr<T>&;

template<typename T> using ptr = T*;
template<typename... T> using vfptr = void (*)(T...);

class cacheline_barrier
  {
  private:
    alignas(max_align_t) array<char, 64U - alignof(max_align_t)> bytes;

  public:
    cacheline_barrier() noexcept = default;
    cacheline_barrier(const cacheline_barrier&) = delete;
    cacheline_barrier& operator=(cacheline_barrier&) = delete;
  };

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
    connection_event_open    = 0,
    connection_event_stream  = 1,
    connection_event_closed  = 2,
  };

enum HTTP_Message_Body_Type : uint8_t
  {
    http_message_body_normal   = 0,
    http_message_body_empty    = 1,
    http_message_body_connect  = 2,
  };

// Base types
class char256;
class uuid;
using guid = uuid;

class Config_File;
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
class HTTP_Server_Session;
class HTTP_Client_Session;
class HTTPS_Server_Session;
class HTTPS_Client_Session;

// HTTP types
class HTTP_DateTime;
class HTTP_Value;
class HTTP_Header_Parser;
struct HTTP_Request_Headers;
class HTTP_Request_Parser;
struct HTTP_Response_Headers;
class HTTP_Response_Parser;
struct WebSocket_Frame_Header;

// Easy types
// Being 'easy' means all callbacks are invoked in fibers and can perform
// async/await operations. These are suitable for agile development.
class Easy_Deflator;
class Easy_Inflator;
class Easy_Timer;
class Easy_UDP_Server;
class Easy_UDP_Client;
class Easy_TCP_Server;
class Easy_TCP_Client;
class Easy_SSL_Server;
class Easy_SSL_Client;
class Easy_HTTP_Server;
class Easy_HTTP_Client;
class Easy_HTTPS_Server;
class Easy_HTTPS_Client;

// Singletons
extern atomic_relaxed<int> exit_signal;
extern class Main_Config& main_config;
extern class Fiber_Scheduler& fiber_scheduler;

extern class Async_Logger& async_logger;
extern class Timer_Driver& timer_driver;
extern class Async_Task_Executor& async_task_executor;
extern class Network_Driver& network_driver;

// Thunks for invocable objects, for type erasure
template<typename... ArgsT>
class Thunk
  {
  public:
    using function_type = void (ArgsT...);
    template<typename T> using is_invocable = ::std::is_invocable<T&, ArgsT&&...>;

  private:
    vfptr<void*, ArgsT&&...> m_func;
    shptr<void> m_obj;

  public:
    // Points this callback to a target object, with its type erased.
    template<typename RealT,
    ROCKET_ENABLE_IF(is_invocable<RealT>::value)>
    explicit
    Thunk(const shptr<RealT>& obj) noexcept
      {
        this->m_func = +[](void* p, ArgsT&&... args) { (*(RealT*) p) (::std::forward<ArgsT>(args)...);  };
        this->m_obj = obj;
      }

    // And this is an optimized overload if the target object is a plain
    // function pointer, which can be stored into `m_obj` directly.
    explicit
    Thunk(function_type* fptr) noexcept
      {
        this->m_func = nullptr;
        this->m_obj = shptr<void>(shptr<int>(), (void*)(intptr_t) fptr);
      }

    // Performs a virtual call to the target object.
    void
    operator()(ArgsT... args) const
      {
        if(this->m_func)
          this->m_func(this->m_obj.get(), ::std::forward<ArgsT>(args)...);
        else
          ((function_type*)(intptr_t) this->m_obj.get()) (::std::forward<ArgsT>(args)...);
      }
  };

// Smart pointers
template<typename ValueT, typename... ArgsT>
ROCKET_ALWAYS_INLINE
uniptr<ValueT>
new_uni(ArgsT&&... args)
  {
    return ::std::make_unique<ValueT>(::std::forward<ArgsT>(args)...);
  }

template<typename ValueT>
ROCKET_ALWAYS_INLINE
uniptr<typename ::std::decay<ValueT>::type>
new_uni(ValueT&& value)
  {
    return ::std::make_unique<typename ::std::decay<ValueT>::type>(::std::forward<ValueT>(value));
  }

template<typename ValueT, typename... ArgsT>
ROCKET_ALWAYS_INLINE
shptr<ValueT>
new_sh(ArgsT&&... args)
  {
    return ::std::make_shared<ValueT>(::std::forward<ArgsT>(args)...);
  }

template<typename ValueT>
ROCKET_ALWAYS_INLINE
shptr<typename ::std::decay<ValueT>::type>
new_sh(ValueT&& value)
  {
    return ::std::make_shared<typename ::std::decay<ValueT>::type>(::std::forward<ValueT>(value));
  }

// Logging with prettification
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
async_logger_check_level(Log_Level level) noexcept;

void
async_logger_enqueue(const Log_Context& ctx, vfptr<cow_string&, const void*> invoke, const void* compose) noexcept;

template<typename... ParamsT>
static ROCKET_ALWAYS_INLINE
bool
async_logger_enqueue_generic(const Log_Context& ctx, const ParamsT&... params) noexcept
  {
    const auto compose = [&](cow_string& sbuf) { ::asteria::format(sbuf, params...);  };
    constexpr auto invoke = +[](cow_string& sbuf, const void* vp) { (*(decltype(compose)*) vp) (sbuf);  };
    noadl::async_logger_enqueue(ctx, invoke, &compose);
    return true;
  }

// Define helper macros that compose log messages. The `TEMPLATE` argument
// shall be a list of string literals in parentheses. Multiple strings are
// joined with line separators.
#define POSEIDON_LOG_GENERIC(LEVEL, TEMPLATE, ...)  \
  (::poseidon::async_logger_check_level(::poseidon::log_level_##LEVEL)  \
   && ::poseidon::async_logger_enqueue_generic(  \
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
