// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FWD_
#define POSEIDON_FWD_

#include "version.h"
#include <rocket/atomic.hpp>
#include <rocket/mutex.hpp>
#include <rocket/recursive_mutex.hpp>
#include <rocket/condition_variable.hpp>
#include <rocket/tinyfmt_str.hpp>
#include <rocket/tinyfmt_ln.hpp>
#include <rocket/unique_posix_fd.hpp>
#include <rocket/unique_posix_file.hpp>
#include <rocket/unique_posix_dir.hpp>
#include <rocket/shared_function.hpp>
#include <rocket/static_char_buffer.hpp>
#include <asteria/value.hpp>
#include <asteria/utils.hpp>
#include <taxon.hpp>
#include <vector>
#include <string>
#include <cxxabi.h>
#include <x86intrin.h>
#include <emmintrin.h>
extern "C++" void poseidon_module_main();  // see below
namespace poseidon {
namespace noadl = poseidon;

// Macros
#define POSEIDON_HIDDEN_X_STRUCT(C, S)  \
  struct __attribute__((__visibility__("hidden"))) C::X_##S  \
    : S { using S::S, S::operator=;  }  // no semicolon

#define POSEIDON_VISIBILITY_HIDDEN   \
  __attribute__((__visibility__("hidden"))) inline

#define POSEIDON_USING  \
  template<typename... Ts> using

#define POSEIDON_SYSCALL_LOOP(...)  \
    __extension__  \
      ({  \
        auto wdLAlUiJ = (__VA_ARGS__);  \
        while(ROCKET_UNEXPECT(wdLAlUiJ < 0) && (errno == EINTR))  \
          wdLAlUiJ = (__VA_ARGS__);  \
        wdLAlUiJ;  \
      })

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

using namespace ::std::literals;
using ::std::chrono::duration;
using ::std::chrono::time_point;
using ::std::chrono::system_clock;
using system_time = system_clock::time_point;
using ::std::chrono::steady_clock;
using steady_time = steady_clock::time_point;

using nanoseconds = ::std::chrono::duration<int64_t, ::std::nano>;
using milliseconds = ::std::chrono::duration<int64_t, ::std::milli>;
using seconds = ::std::chrono::duration<int, ::std::ratio<1>>;
using minutes = ::std::chrono::duration<int, ::std::ratio<60>>;
using hours = ::std::chrono::duration<int, ::std::ratio<3600>>;
using days = ::std::chrono::duration<int, ::std::ratio<86400>>;
using weeks = ::std::chrono::duration<int, ::std::ratio<604800>>;

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
using ::rocket::array;
using ::rocket::cow_vector;
using ::asteria::cow_dictionary;
using ::rocket::cow_hashmap;
using ::rocket::static_vector;
using ::rocket::cow_string;
using ::rocket::cow_bstring;
using ::rocket::cow_u16string;
using ::rocket::cow_u32string;
using ::rocket::prehashed_string;
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

POSEIDON_USING cow_bivector = ::rocket::cow_vector<::std::pair<Ts...>>;
POSEIDON_USING opt = ::rocket::optional<Ts...>;
POSEIDON_USING uniptr = ::std::unique_ptr<Ts...>;
POSEIDON_USING shptr = ::std::shared_ptr<Ts...>;
POSEIDON_USING wkptr = ::std::weak_ptr<Ts...>;

using cow_stringR = const ::rocket::cow_string&;
POSEIDON_USING shptrR = const ::std::shared_ptr<Ts...>&;
using charbuf_16 = ::rocket::static_char_buffer<16>;
using charbuf_256 = ::rocket::static_char_buffer<256>;

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
using ::rocket::make_unique_handle;
using ::rocket::min;
using ::rocket::max;
using ::rocket::clamp;
using ::rocket::clamp_cast;
using ::rocket::is_any_of;
using ::rocket::is_none_of;
using ::rocket::all_of;
using ::rocket::any_of;
using ::rocket::none_of;
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

using ::asteria::format;
using ::asteria::format_string;

template<typename xValue, typename... xArgs>
ROCKET_ALWAYS_INLINE
uniptr<xValue>
new_uni(xArgs&&... args)
  {
    return ::std::make_unique<xValue>(forward<xArgs>(args)...);
  }

template<typename xValue>
ROCKET_ALWAYS_INLINE
uniptr<typename ::std::decay<xValue>::type>
new_uni(xValue&& value)
  {
    return ::std::make_unique<typename ::std::decay<xValue>::type>(forward<xValue>(value));
  }

template<typename xValue, typename... xArgs>
ROCKET_ALWAYS_INLINE
shptr<xValue>
new_sh(xArgs&&... args)
  {
    return ::std::make_shared<xValue>(forward<xArgs>(args)...);
  }

template<typename xValue>
ROCKET_ALWAYS_INLINE
shptr<typename ::std::decay<xValue>::type>
new_sh(xValue&& value)
  {
    return ::std::make_shared<typename ::std::decay<xValue>::type>(forward<xValue>(value));
  }

struct cacheline_barrier
  {
    static constexpr size_t alignment = alignof(max_align_t);
    static constexpr size_t size = 64UL - alignof(max_align_t);

    alignas(alignment) char bytes[size];

    cacheline_barrier() noexcept = default;
    cacheline_barrier(const cacheline_barrier&) { }
    cacheline_barrier& operator=(const cacheline_barrier&) { return *this; }
  };

struct chars_view
  {
    const char* p;
    size_t n;

    constexpr chars_view(nullptr_t = nullptr) noexcept
      : p(nullptr), n(0U)  { }

    constexpr chars_view(const char* xp, size_t xn) noexcept
      : p(xp), n(xn)  { }

    constexpr chars_view(const char* xs) noexcept
      : p(xs), n(xs ? ::rocket::xstrlen(xs) : 0U)  { }

    template<typename traitsT, typename allocT>
    constexpr chars_view(const ::std::basic_string<char, traitsT, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<typename allocT>
    constexpr chars_view(const ::std::vector<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

#ifdef __cpp_lib_string_view
    template<typename traitsT>
    constexpr chars_view(const ::std::basic_string_view<char, traitsT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }
#endif

    constexpr chars_view(const ::rocket::shallow_string rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<size_t N>
    constexpr chars_view(const char (*ps)[N]) noexcept
      : p(*ps), n((ROCKET_ASSERT(*(*ps + N - 1) == '\0'), N - 1))  { }

    template<typename allocT>
    constexpr chars_view(const ::rocket::basic_cow_string<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<typename allocT>
    constexpr chars_view(const ::rocket::basic_tinybuf_str<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<typename allocT>
    constexpr chars_view(const ::rocket::basic_tinyfmt_str<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<typename allocT>
    constexpr chars_view(const ::rocket::basic_linear_buffer<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<typename allocT>
    constexpr chars_view(const ::rocket::basic_tinybuf_ln<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<typename allocT>
    constexpr chars_view(const ::rocket::basic_tinyfmt_ln<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<typename allocT>
    constexpr chars_view(const ::rocket::cow_vector<char, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    template<size_t N, typename allocT>
    constexpr chars_view(const ::rocket::static_vector<char, N, allocT>& rs) noexcept
      : p(rs.data()), n(rs.size())  { }

    // Returns the first character. Depending on the nature of the source string,
    // reading one character past the end might be allowed, so we don't check
    // whether `n` equals zero here.
    constexpr
    char
    operator*() const noexcept
      { return *(this->p);  }

    constexpr
    char
    operator[](size_t index) const noexcept
      { return ROCKET_ASSERT(index <= this->n), *(this->p + index);  }

    // Moves the view to the left.
    constexpr
    chars_view
    operator<<(size_t dist) const noexcept
      { return chars_view(this->p - dist, this->n + dist);  }

    constexpr
    chars_view&
    operator<<=(size_t dist) & noexcept
      { return *this = *this << dist;  }

    // Moves the view to the right.
    constexpr
    chars_view
    operator>>(size_t dist) const noexcept
      { return chars_view(this->p + dist, this->n - dist);  }

    constexpr
    chars_view&
    operator>>=(size_t dist) & noexcept
      { return *this = *this >> dist;  }

    // Makes a copy.
    explicit operator cow_string() const
      { return cow_string(this->p, this->n);  }
  };

inline
tinyfmt&
operator<<(tinyfmt& fmt, chars_view data)
  { return fmt.putn(data.p, data.n);  }

inline
chars_view
sview(const char* str) noexcept
  { return chars_view(str, ::strlen(str));  }

inline
chars_view
snview(const char* str, size_t n) noexcept
  { return chars_view(str, ::strnlen(str, n));  }

// Protocol constants
enum HTTP_Method : uint64_t
  {
    http_NULL      = ROCKET_HTOBE64(0x0000000000000000),
    http_OPTIONS   = ROCKET_HTOBE64(0x4F5054494F4E5300),
    http_GET       = ROCKET_HTOBE64(0x4745540000000000),
    http_HEAD      = ROCKET_HTOBE64(0x4845414400000000),
    http_POST      = ROCKET_HTOBE64(0x504F535400000000),
    http_PUT       = ROCKET_HTOBE64(0x5055540000000000),
    http_DELETE    = ROCKET_HTOBE64(0x44454C4554450000),
    http_TRACE     = ROCKET_HTOBE64(0x5452414345000000),
    http_CONNECT   = ROCKET_HTOBE64(0x434F4E4E45435400),
    http_PATCH     = ROCKET_HTOBE64(0x5041544348000000),
  };

enum HTTP_Status : uint16_t
  {
    http_status_null                             =   0,
    http_status_continue                         = 100,
    http_status_switching_protocols              = 101,
    http_status_ok                               = 200,
    http_status_connection_established           = 200,
    http_status_created                          = 201,
    http_status_accepted                         = 202,
    http_status_nonauthoritative_information     = 203,
    http_status_no_content                       = 204,
    http_status_reset_content                    = 205,
    http_status_partial_content                  = 206,
    http_status_multiple_choices                 = 300,
    http_status_moved_permanently                = 301,
    http_status_found                            = 302,
    http_status_see_other                        = 303,
    http_status_not_modified                     = 304,
    http_status_use_proxy                        = 305,
    http_status_temporary_redirect               = 307,
    http_status_bad_request                      = 400,
    http_status_unauthorized                     = 401,
    http_status_payment_required                 = 402,
    http_status_forbidden                        = 403,
    http_status_not_found                        = 404,
    http_status_method_not_allowed               = 405,
    http_status_not_acceptable                   = 406,
    http_status_proxy_authentication_required    = 407,
    http_status_request_timeout                  = 408,
    http_status_conflict                         = 409,
    http_status_gone                             = 410,
    http_status_length_required                  = 411,
    http_status_precondition_failed              = 412,
    http_status_payload_too_large                = 413,
    http_status_uri_too_long                     = 414,
    http_status_unsupported_media_type           = 415,
    http_status_range_not_satisfiable            = 416,
    http_status_expectation_failed               = 417,
    http_status_upgrade_required                 = 426,
    http_status_internal_server_error            = 500,
    http_status_not_implemented                  = 501,
    http_status_bad_gateway                      = 502,
    http_status_service_unavailable              = 503,
    http_status_gateway_timeout                  = 504,
    http_status_http_version_not_supported       = 505,
  };

enum WebSocket_Opcode : uint8_t
  {
    websocket_CONTINUATION   =  0,
    websocket_TEXT           =  1,
    websocket_BINARY         =  2,
    websocket_CLOSE          =  8,
    websocket_PING           =  9,
    websocket_PONG           = 10,
  };

enum WebSocket_Status : uint16_t
  {
    websocket_status_null                  =    0,
    websocket_status_normal_closure        = 1000,
    websocket_status_going_away            = 1001,
    websocket_status_protocol_error        = 1002,
    websocket_status_not_acceptable        = 1003,
    websocket_status_no_status_code        = 1005,
    websocket_status_no_close_frame        = 1006,
    websocket_status_message_data_error    = 1007,
    websocket_status_policy_violation      = 1008,
    websocket_status_message_too_large     = 1009,
    websocket_status_extension_required    = 1010,
    websocket_status_unexpected_error      = 1011,
    websocket_status_tls_error             = 1015,
  };

// Base types
class UUID;
class DateTime;
class Config_File;
class Abstract_Timer;
class Abstract_Task;
class Abstract_Deflator;
class Abstract_Inflator;

// Fiber types
class Abstract_Fiber;
class Abstract_Future;
class DNS_Query_Future;
class Read_File_Future;
class MySQL_Query_Future;
class MySQL_Check_Table_Future;
class Mongo_Query_Future;
class Redis_Query_Future;
class Redis_Scan_and_Get_Future;

// Socket types
enum IP_Address_Class : uint8_t;
enum Socket_State : uint8_t;
enum HTTP_Payload_Type : uint8_t;
class IPv6_Address;
class Abstract_Socket;
class TCP_Acceptor;
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
class DNS_Connect_Task;

// HTTP and WebSocket types
class HTTP_Value;
class HTTP_Field_Name;
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
enum MySQL_Column_Type : uint8_t;
enum MySQL_Index_Type : uint8_t;
enum MySQL_Engine_Type : uint8_t;
enum MySQL_Value_Type : uint8_t;
class MySQL_Table_Structure;
class MySQL_Value;
class MySQL_Connection;

// MongoDB types
enum Mongo_Value_Type : uint8_t;
class Mongo_Value;
using Mongo_Array = ::asteria::cow_vector<Mongo_Value>;
using Mongo_Document = ::asteria::cow_bivector<cow_string, Mongo_Value>;
class Mongo_Connection;

// Redis types
enum Redis_Value_Type : uint8_t;
class Redis_Value;
using Redis_Array = ::asteria::cow_vector<Redis_Value>;
class Redis_Connection;

// Easy types
// Being 'easy' means all callbacks are invoked in fibers and can perform
// async/await operations. These are suitable for agile development.
enum Easy_Stream_Event : uint8_t;  // TCP, SSL
enum Easy_HTTP_Event : uint8_t;  // HTTP, HTTPS
enum Easy_HWS_Event : uint8_t;  // HTTP+WS, HTTPS+WSS
enum Easy_WS_Event : uint8_t;  // WS, WSS
class Easy_Timer;
class Easy_UDP_Server;
class Easy_UDP_Client;
class Easy_TCP_Server;
class Easy_HTTP_Server;
class Easy_HWS_Server;
class Easy_WS_Server;
class Easy_SSL_Server;
class Easy_HTTPS_Server;
class Easy_HWSS_Server;
class Easy_WSS_Server;
class Easy_TCP_Client;
class Easy_HTTP_Client;
class Easy_WS_Client;
class Easy_SSL_Client;
class Easy_HTTPS_Client;
class Easy_WSS_Client;

// Singletons
extern const ::locale_t c_locale;
extern atomic_relaxed<int> exit_signal;
extern class Main_Config& main_config;
extern class Logger& logger;

extern class Timer_Driver& timer_driver;
extern class Task_Executor& task_executor;
extern class Network_Driver& network_driver;
extern class Fiber_Scheduler& fiber_scheduler;

extern class MySQL_Connector& mysql_connector;
extern class Mongo_Connector& mongo_connector;
extern class Redis_Connector& redis_connector;

// Entry point procedure for modules
// This function is to be defined by users. It will be called after a module
// is loaded.
using ::poseidon_module_main;

}  // namespace poseidon
#endif
