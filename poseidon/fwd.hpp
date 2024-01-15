// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FWD_
#define POSEIDON_FWD_

#include "version.h"
#include <asteria/utils.hpp>
#include <rocket/atomic.hpp>
#include <rocket/mutex.hpp>
#include <rocket/recursive_mutex.hpp>
#include <rocket/condition_variable.hpp>
#include <rocket/tinyfmt_str.hpp>
#include <rocket/tinyfmt_ln.hpp>
#include <rocket/unique_posix_fd.hpp>
#include <rocket/unique_posix_file.hpp>
#include <rocket/unique_posix_dir.hpp>
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
    }  \
    // no semicolon

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

using namespace ::std::literals;
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
using ::rocket::move;
using ::rocket::forward;
using ::rocket::forward_as_tuple;
using ::rocket::exchange;
using ::rocket::size;
using ::rocket::ssize;
using ::rocket::static_pointer_cast;
using ::rocket::dynamic_pointer_cast;
using ::rocket::const_pointer_cast;
using ::rocket::sref;
using ::rocket::nullopt;

using ::rocket::xstrlen;
using ::rocket::xstrchr;
using ::rocket::xstrcmp;
using ::rocket::xstreq;
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

template<size_t Nc> using char_array = array<char, Nc>;
template<size_t Nc> using uchar_array = array<unsigned char, Nc>;

// utilities
struct cacheline_barrier
  {
    static constexpr size_t align = alignof(max_align_t);
    static constexpr size_t size = 64UL - alignof(max_align_t);
    alignas(align) uchar_array<size> bytes;

    // All contents are padding, so these functions do nothing.
    cacheline_barrier() noexcept { }
    cacheline_barrier(const cacheline_barrier&) { }
    cacheline_barrier& operator=(const cacheline_barrier&) { return *this;  }
  };

template<typename... ArgsT>
class thunk
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
    thunk(const shptr<RealT>& obj) noexcept
      {
        this->m_func = +[](void* p, ArgsT&&... args) { (*(RealT*) p) (forward<ArgsT>(args)...);  };
        this->m_obj = obj;
      }

    // And this is an optimized overload if the target object is a plain
    // function pointer, which can be stored into `m_obj` directly.
    explicit
    thunk(function_type* fptr) noexcept
      {
        this->m_func = nullptr;
        this->m_obj = shptr<void>(shptr<int>(), (void*)(intptr_t) fptr);
      }

    // Performs a virtual call to the target object.
    void
    operator()(ArgsT... args) const
      {
        if(this->m_func)
          this->m_func(this->m_obj.get(), forward<ArgsT>(args)...);
        else
          ((function_type*)(intptr_t) this->m_obj.get()) (forward<ArgsT>(args)...);
      }
  };

class char256
  {
  private:
    union {
      char m_data[8];
      ::std::aligned_storage<256>::type m_stor;
    };

  public:
    // Constructs a null-terminated string of zero characters.
    // This constructor is not explicit as it doesn't allocate memory.
    constexpr
    char256() noexcept
      :
        m_data()
      { }

    // Constructs a null-terminated string.
    // This constructor is not explicit as it doesn't allocate memory.
    constexpr
    char256(const char* str_opt)
      :
        m_data()
      {
        const char* str = str_opt ? str_opt : "";
        size_t len = ::rocket::xstrlen(str);
        if(len >= 256)
          ::rocket::sprintf_and_throw<::std::length_error>(
              "char256: string `%s` (length `%lld`) too long",
              str, (long long) len);

        // XXX: This should be `xmempcpy()` but clang doesn't like it?
        ::rocket::details_xstring::maybe_constexpr::ymempcpy(this->m_data, str, len + 1);
      }

  public:
    // Returns a pointer to internal storage so a buffer can be passed as
    // an argument for `char*`.
    constexpr operator
    const char*() const noexcept
      { return this->m_data;  }

    operator
    char*() noexcept
      { return this->m_data;  }

    constexpr
    const char*
    c_str() const noexcept
      { return this->m_data;  }
  };

inline
tinyfmt&
operator<<(tinyfmt& fmt, const char256& cbuf)
  { return fmt.putn(cbuf.c_str(), ::rocket::xstrlen(cbuf.c_str()));  }

struct chars_view
  {
    const char* p;
    size_t n;

    constexpr
    chars_view(nullptr_t = nullptr) noexcept
      :
        p(nullptr), n(0U)
      { }

    constexpr
    chars_view(const char* xp, size_t xn) noexcept
      :
        p(xp), n(xn)
      { }

    constexpr
    chars_view(const char* xs) noexcept
      :
        p(xs), n(xs ? ::rocket::xstrlen(xs) : 0U)
      { }

    template<typename traitsT, typename allocT>
    constexpr
    chars_view(const ::std::basic_string<char, traitsT, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::std::vector<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

#ifdef __cpp_lib_string_view
    template<typename traitsT>
    constexpr
    chars_view(const ::std::basic_string_view<char, traitsT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }
#endif  // __cpp_lib_string_view

    constexpr
    chars_view(const ::rocket::shallow_string rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::rocket::basic_cow_string<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::rocket::basic_tinybuf_str<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::rocket::basic_tinyfmt_str<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::rocket::basic_linear_buffer<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::rocket::basic_tinybuf_ln<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::rocket::basic_tinyfmt_ln<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<typename allocT>
    constexpr
    chars_view(const ::rocket::cow_vector<char, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    template<size_t N, typename allocT>
    constexpr
    chars_view(const ::rocket::static_vector<char, N, allocT>& rs) noexcept
      :
        p(rs.data()), n(rs.size())
      { }

    // Returns the first character. Depending on the nature of the source string,
    // reading one character past the end might be allowed, so we don't check
    // whether `n` equals zero here.
    constexpr
    char
    operator*() const noexcept
      {
        return *(this->p);
      }

    constexpr
    char
    operator[](size_t index) const noexcept
      {
        ROCKET_ASSERT(index <= this->n);
        return this->p[index];
      }

    // Moves the view to the left.
    constexpr
    chars_view
    operator<<(size_t dist) const noexcept
      {
        return chars_view(this->p - dist, this->n + dist);
      }

    chars_view
    operator<<=(size_t dist) & noexcept
      {
        return *this = *this << dist;
      }

    // Moves the view to the right, stopping at the boundary.
    constexpr
    chars_view
    operator>>(size_t dist) const noexcept
      {
        ROCKET_ASSERT(dist <= this->n);
        return chars_view(this->p + dist, this->n - dist);
      }

    chars_view
    operator>>=(size_t dist) & noexcept
      {
        return *this = *this >> dist;
      }

    // Makes a copy.
    cow_string
    str() const
      {
        return cow_string(this->p, this->n);
      }
  };

inline
tinyfmt&
operator<<(tinyfmt& fmt, chars_view data)
  { return fmt.putn(data.p, data.n);  }

template<typename ValueT, typename... ArgsT>
ROCKET_ALWAYS_INLINE
uniptr<ValueT>
new_uni(ArgsT&&... args)
  { return ::std::make_unique<ValueT>(forward<ArgsT>(args)...);  }

template<typename ValueT>
ROCKET_ALWAYS_INLINE
uniptr<typename ::std::decay<ValueT>::type>
new_uni(ValueT&& value)
  { return ::std::make_unique<typename ::std::decay<ValueT>::type>(forward<ValueT>(value));  }

template<typename ValueT, typename... ArgsT>
ROCKET_ALWAYS_INLINE
shptr<ValueT>
new_sh(ArgsT&&... args)
  { return ::std::make_shared<ValueT>(forward<ArgsT>(args)...);  }

template<typename ValueT>
ROCKET_ALWAYS_INLINE
shptr<typename ::std::decay<ValueT>::type>
new_sh(ValueT&& value)
  { return ::std::make_shared<typename ::std::decay<ValueT>::type>(forward<ValueT>(value));  }

// Constants
enum zlib_Format : uint8_t
  {
    zlib_deflate  = 0,
    zlib_raw      = 1,
    zlib_gzip     = 2,
  };

enum Async_State : uint8_t
  {
    async_pending    = 0,
    async_suspended  = 1,
    async_running    = 2,
    async_finished   = 3,
  };

// Base types
class UUID;
using GUID = UUID;
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
enum IP_Address_Class : uint8_t;
enum Socket_State : uint8_t;
enum HTTP_Payload_Type : uint8_t;
enum WebSocket_OpCode : uint8_t;
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
class WS_Server_Session;
class WS_Client_Session;
class WSS_Server_Session;
class WSS_Client_Session;
class Async_Connect;

// HTTP types
class HTTP_DateTime;
class HTTP_Value;
class HTTP_Header_Parser;
class HTTP_Query_Parser;
struct HTTP_Request_Headers;
class HTTP_Request_Parser;
struct HTTP_Response_Headers;
class HTTP_Response_Parser;
struct WebSocket_Frame_Header;
class WebSocket_Frame_Parser;
class WebSocket_Deflator;

// MySQL types
enum MySQL_Data_Type : uint8_t;
enum MySQL_Engine_Type : uint8_t;
class MySQL_Table;

// Easy types
// Being 'easy' means all callbacks are invoked in fibers and can perform
// async/await operations. These are suitable for agile development.
enum Easy_Stream_Event : uint8_t;  // TCP, SSL
enum Easy_HTTP_Event : uint8_t;  // HTTP, HTTPS
enum Easy_WS_Event : uint8_t;  // WS, WSS
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
class Easy_WS_Server;
class Easy_WS_Client;
class Easy_WSS_Server;
class Easy_WSS_Client;

// Singletons
extern atomic_relaxed<int> exit_signal;
extern class Main_Config& main_config;
extern class Fiber_Scheduler& fiber_scheduler;

extern class Async_Logger& async_logger;
extern class Timer_Driver& timer_driver;
extern class Async_Task_Executor& async_task_executor;
extern class Network_Driver& network_driver;

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
async_logger_enqueue(const Log_Context& ctx, vfptr<cow_string&, void*> invoke, void* compose) noexcept;

// Define helper macros that compose log messages. The `TEMPLATE` argument shall
// be a list of string literals in parentheses. Multiple strings are joined with
// line separators. `format()` and `async_logger_enqueue()` are found via ADL.
#define POSEIDON_LOG_G_(LEVEL, TEMPLATE, ...)  \
    (::poseidon::async_logger_check_level(::poseidon::log_level_##LEVEL)  \
      &&  \
      __extension__ ({  \
        auto compose = [&](::rocket::cow_string& sbuf)  \
          { format(sbuf, (::asteria::make_string_template TEMPLATE), ##__VA_ARGS__);  };  \
        constexpr auto invoke = +[](::rocket::cow_string& sbuf, void* vp)  \
          { (*(decltype(compose)*) vp) (sbuf);  };  \
        static const ::poseidon::Log_Context ctx =  \
          { __FILE__, __LINE__, ::poseidon::log_level_##LEVEL, ROCKET_FUNCSIG };  \
        async_logger_enqueue(ctx, invoke, &compose);  \
        true;  \
      }))

#define POSEIDON_LOG_FATAL(...)   POSEIDON_LOG_G_(fatal, __VA_ARGS__)
#define POSEIDON_LOG_ERROR(...)   POSEIDON_LOG_G_(error, __VA_ARGS__)
#define POSEIDON_LOG_WARN(...)    POSEIDON_LOG_G_(warn,  __VA_ARGS__)
#define POSEIDON_LOG_INFO(...)    POSEIDON_LOG_G_(info,  __VA_ARGS__)
#define POSEIDON_LOG_DEBUG(...)   POSEIDON_LOG_G_(debug, __VA_ARGS__)
#define POSEIDON_LOG_TRACE(...)   POSEIDON_LOG_G_(trace, __VA_ARGS__)

// Evaluates an expression. If an exception is thrown, a message is printed but
// the exception itself is caught and ignored.
#define POSEIDON_CATCH_ALL(...)  \
    (__extension__ ({  \
      try { (void) (__VA_ARGS__);  }  \
      catch(::std::exception& zeew2aeY)  \
      { POSEIDON_LOG_ERROR(("Ignoring exception: $1"), zeew2aeY);  }  \
    }))

}  // namespace poseidon
#endif
