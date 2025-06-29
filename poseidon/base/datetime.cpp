// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "datetime.hpp"
#include "../utils.hpp"
#include <time.h>
namespace poseidon {

DateTime::
DateTime(chars_view str)
  {
    if(this->parse(str) != str.n)
      POSEIDON_THROW(("Could not parse date/time string `$1`"), str);
  }

size_t
DateTime::
parse_rfc1123_partial(const char* str)
  {
    // `Sun, 06 Nov 1994 08:49:37 GMT`
    char temp[32];
    size_t len = ::strnlen(str, 29);
    ::memcpy(temp, str, len);
    temp[len] = 0;

    struct tm tm;
    if(::strptime_l(temp, "%a, %d %b %Y %T GMT", &tm, c_locale) != temp + len)
      return 0;

    tm.tm_isdst = 0;
    this->m_tp = system_clock::from_time_t(::timegm(&tm));
    return len;
  }

size_t
DateTime::
parse_rfc850_partial(const char* str)
  {
    // `Sunday, 06-Nov-94 08:49:37 GMT`
    char temp[40];
    size_t len = ::strnlen(str, 33);
    ::memcpy(temp, str, len);
    temp[len] = 0;

    struct tm tm;
    if(::strptime_l(temp, "%A, %d-%b-%y %T GMT", &tm, c_locale) != temp + len)
      return 0;

    tm.tm_isdst = 0;
    this->m_tp = system_clock::from_time_t(::timegm(&tm));
    return len;
  }

size_t
DateTime::
parse_asctime_partial(const char* str)
  {
    // `Sun Nov  6 08:49:37 1994`
    char temp[32];
    size_t len = ::strnlen(str, 24);
    ::memcpy(temp, str, len);
    temp[len] = 0;

    struct tm tm;
    if(::strptime_l(temp, "%a %b %e %T %Y", &tm, c_locale) != temp + len)
      return 0;

    tm.tm_isdst = 0;
    this->m_tp = system_clock::from_time_t(::timegm(&tm));
    return len;
  }

size_t
DateTime::
parse_cookie_partial(const char* str)
  {
    // `Sun, 06-Nov-1994 08:49:37 GMT`
    char temp[32];
    size_t len = ::strnlen(str, 29);
    ::memcpy(temp, str, len);
    temp[len] = 0;

    struct tm tm;
    if(::strptime_l(temp, "%a, %d-%b-%Y %T GMT", &tm, c_locale) != temp + len)
      return 0;

    tm.tm_isdst = 0;
    this->m_tp = system_clock::from_time_t(::timegm(&tm));
    return len;
  }

size_t
DateTime::
parse_iso8601_partial(const char* str)
  {
    // `1994-11-06T08:49:37Z`
    char temp[32];
    size_t len = ::strnlen(str, 20);
    ::memcpy(temp, str, len);
    temp[len] = 0;

    struct tm tm;
    if(::strptime_l(temp, "%Y-%m-%dT%TZ", &tm, c_locale) != temp + len)
      return 0;

    tm.tm_isdst = 0;
    this->m_tp = system_clock::from_time_t(::timegm(&tm));
    return len;
  }

size_t
DateTime::
parse(chars_view str)
  {
    // A string with an erroneous length will not be accepted, so we just need to
    // check for possibilities by `str.n`.
    if(str.n >= 29)
      if(size_t aclen = this->parse_rfc1123_partial(str.p))
        return aclen;

    if(str.n >= 30)
      if(size_t aclen = this->parse_rfc850_partial(str.p))
        return aclen;

    if(str.n >= 24)
      if(size_t aclen = this->parse_asctime_partial(str.p))
        return aclen;

    if(str.n >= 29)
      if(size_t aclen = this->parse_cookie_partial(str.p))
        return aclen;

    if(str.n >= 20)
      if(size_t aclen = this->parse_iso8601_partial(str.p))
        return aclen;

    return 0;
  }

size_t
DateTime::
print_rfc1123_partial(char* str) const noexcept
  {
    // `Sun, 06 Nov 1994 08:49:37 GMT`
    struct timespec ts;
    timespec_from_system_time(ts, this->m_tp);
    ts.tv_sec = ::rocket::clamp_cast<::time_t>(ts.tv_sec, -2208988800, 253402300799);
    struct tm tm;
    ::gmtime_r(&(ts.tv_sec), &tm);

    size_t len = ::strftime_l(str, 30, "%a, %d %b %Y %T GMT", &tm, c_locale);
    ROCKET_ASSERT(len == 29);
    return len;
  }

size_t
DateTime::
print_rfc850_partial(char* str) const noexcept
  {
    // `Sunday, 06-Nov-94 08:49:37 GMT`
    struct timespec ts;
    timespec_from_system_time(ts, this->m_tp);
    ts.tv_sec = ::rocket::clamp_cast<::time_t>(ts.tv_sec, -2208988800, 253402300799);
    struct tm tm;
    ::gmtime_r(&(ts.tv_sec), &tm);

    size_t len = ::strftime_l(str, 34, "%A, %d-%b-%y %T GMT", &tm, c_locale);
    ROCKET_ASSERT((len >= 30) && (len <= 33));
    return len;
  }

size_t
DateTime::
print_asctime_partial(char* str) const noexcept
  {
    // `Sun Nov  6 08:49:37 1994`
    struct timespec ts;
    timespec_from_system_time(ts, this->m_tp);
    ts.tv_sec = ::rocket::clamp_cast<::time_t>(ts.tv_sec, -2208988800, 253402300799);
    struct tm tm;
    ::gmtime_r(&(ts.tv_sec), &tm);

    size_t len = ::strftime_l(str, 25, "%a %b %e %T %Y", &tm, c_locale);
    ROCKET_ASSERT(len == 24);
    return len;
  }

size_t
DateTime::
print_cookie_partial(char* str) const noexcept
  {
    // `Sun, 06-Nov-1994 08:49:37 GMT`
    struct timespec ts;
    timespec_from_system_time(ts, this->m_tp);
    ts.tv_sec = ::rocket::clamp_cast<::time_t>(ts.tv_sec, -2208988800, 253402300799);
    struct tm tm;
    ::gmtime_r(&(ts.tv_sec), &tm);

    size_t len = ::strftime_l(str, 30, "%a, %d-%b-%Y %T GMT", &tm, c_locale);
    ROCKET_ASSERT(len == 29);
    return len;
  }

size_t
DateTime::
print_iso8601_partial(char* str) const noexcept
  {
    // `1994-11-06T08:49:37Z`
    struct timespec ts;
    timespec_from_system_time(ts, this->m_tp);
    ts.tv_sec = ::rocket::clamp_cast<::time_t>(ts.tv_sec, -2208988800, 253402300799);
    struct tm tm;
    ::gmtime_r(&(ts.tv_sec), &tm);

    size_t len = ::strftime_l(str, 21, "%Y-%m-%dT%TZ", &tm, c_locale);
    ROCKET_ASSERT(len == 20);
    return len;
  }

size_t
DateTime::
print_iso8601_ns_partial(char* str) const noexcept
  {
    // `1994-11-06T08:49:37.123456789Z`
    struct timespec ts;
    timespec_from_system_time(ts, this->m_tp);
    ts.tv_sec = ::rocket::clamp_cast<::time_t>(ts.tv_sec, -2208988800, 253402300799);
    struct tm tm;
    ::gmtime_r(&(ts.tv_sec), &tm);

    ::rocket::ascii_numput nump;
    nump.put_DU(static_cast<uint32_t>(ts.tv_nsec), 9);

    size_t len = ::strftime_l(str, 31, "%Y-%m-%dT%T.mmmuuunnnZ", &tm, c_locale);
    ROCKET_ASSERT(len == 30);
    ::memcpy(str + 20, nump.data(), 9);
    return len;
  }

tinyfmt&
DateTime::
print_to(tinyfmt& fmt) const
  {
    char str[64];
    size_t len = this->print_iso8601_ns_partial(str);
    return fmt.putn(str, len);
  }

cow_string
DateTime::
to_string() const
  {
    char str[64];
    size_t len = this->print_iso8601_ns_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
