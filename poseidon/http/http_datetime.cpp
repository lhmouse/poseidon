// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_datetime.hpp"
#include "../utils.hpp"
#include <time.h>
namespace poseidon {
namespace {

/* https://www.rfc-editor.org/rfc/rfc2616#section-3.3.1

   Copyright (C) The Internet Society (1999).  All Rights Reserved.

   This document and translations of it may be copied and furnished to
   others, and derivative works that comment on or otherwise explain it
   or assist in its implementation may be prepared, copied, published
   and distributed, in whole or in part, without restriction of any
   kind, provided that the above copyright notice and this paragraph are
   included on all such copies and derivative works.  However, this
   document itself may not be modified in any way, such as by removing
   the copyright notice or references to the Internet Society or other
   Internet organizations, except as needed for the purpose of
   developing Internet standards in which case the procedures for
   copyrights defined in the Internet Standards process must be
   followed, or as required to translate it into languages other than
   English.

   The limited permissions granted above are perpetual and will not be
   revoked by the Internet Society or its successors or assigns.

   This document and the information contained herein is provided on an
   "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET ENGINEERING
   TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
   BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION
   HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

3.3 Date/Time Formats

3.3.1 Full Date

       HTTP-date    = rfc1123-date | rfc850-date | asctime-date
       rfc1123-date = wkday "," SP date1 SP time SP "GMT"
       rfc850-date  = weekday "," SP date2 SP time SP "GMT"
       asctime-date = wkday SP date3 SP time SP 4DIGIT
       date1        = 2DIGIT SP month SP 4DIGIT
                      ; day month year (e.g., 02 Jun 1982)
       date2        = 2DIGIT "-" month "-" 2DIGIT
                      ; day-month-year (e.g., 02-Jun-82)
       date3        = month SP ( 2DIGIT | ( SP 1DIGIT ))
                      ; month day (e.g., Jun  2)
       time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT
                      ; 00:00:00 - 23:59:59
       wkday        = "Mon" | "Tue" | "Wed"
                    | "Thu" | "Fri" | "Sat" | "Sun"
       weekday      = "Monday" | "Tuesday" | "Wednesday"
                    | "Thursday" | "Friday" | "Saturday" | "Sunday"
       month        = "Jan" | "Feb" | "Mar" | "Apr"
                    | "May" | "Jun" | "Jul" | "Aug"
                    | "Sep" | "Oct" | "Nov" | "Dec"
*/

constexpr char s_2digit[100][2] =
  {
    '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
    '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
    '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
    '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
    '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
    '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
    '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
    '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
    '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
    '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9',
  };

constexpr char s_sp1digit[100][2] =
  {
    ' ','0',' ','1',' ','2',' ','3',' ','4',' ','5',' ','6',' ','7',' ','8',' ','9',
    '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
    '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
    '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
    '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
    '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
    '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
    '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
    '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
    '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9',
  };

constexpr char s_weekday[7][12] =
  {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
  };

constexpr char s_month[12][4] =
  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

inline
bool
do_match(const char*& rptr_out, const char* cstr, size_t len)
  {
    // If a previous match has failed, don't do anything.
    if(rptr_out == nullptr)
      return false;

    if(::memcmp(rptr_out, cstr, len) == 0) {
      // A match has been found, so move the read pointer past it.
      rptr_out += len;
      return true;
    }

    // No match has been found, so mark this as a failure.
    rptr_out = nullptr;
    return false;
  }

template<uint32_t N, uint32_t S>
inline
bool
do_match(const char*& rptr_out, int& add_to_value, const char (&cstrs)[N][S], int limit)
  {
    // If a previous match has failed, don't do anything.
    if(rptr_out == nullptr)
      return false;

    for(uint32_t k = 0;  k != N;  ++k) {
      size_t len = (limit >= 0) ? (uint32_t) limit : ::strlen(cstrs[k]);
      if(::memcmp(rptr_out, cstrs[k], len) == 0) {
        // A match has been found, so move the read pointer past it.
        rptr_out += len;
        add_to_value += (int) k;
        return true;
      }
    }

    // No match has been found, so mark this as a failure.
    rptr_out = nullptr;
    return false;
  }

}  // namespace

const HTTP_DateTime http_datetime_min = (unix_time)(days) 0;
const HTTP_DateTime http_datetime_max = (unix_time)(days) 2932532;

HTTP_DateTime::
HTTP_DateTime(const char* str, size_t len)
  {
    size_t r = this->parse(str, len);
    if(r != len)
      POSEIDON_THROW((
          "Could not parse HTTP data/time string `$1`"),
          cow_string(str, len));
  }

HTTP_DateTime::
HTTP_DateTime(const char* str)
  {
    size_t r = this->parse(str, ::strlen(str));
    if(str[r] != 0)
      POSEIDON_THROW((
          "Could not parse HTTP data/time string `$1`"),
          str);
  }

HTTP_DateTime::
HTTP_DateTime(cow_stringR str)
  {
    size_t r = this->parse(str.data(), str.size());
    if(r != str.size())
      POSEIDON_THROW((
          "Could not parse HTTP data/time string `$1`"),
          str);
  }

size_t
HTTP_DateTime::
parse_rfc1123_partial(const char* str)
  {
    const char* rptr = str;
    ::tm tm = { };

    // `Sun, 06 Nov 1994 08:49:37 GMT`
    do_match(rptr, tm.tm_wday, s_weekday, 3);
    do_match(rptr, ", ", 2);
    do_match(rptr, tm.tm_mday, s_2digit, 2);
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_mon, s_month, 3);
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_year, s_2digit, 2);
    tm.tm_year = tm.tm_year * 100 - 1900;
    do_match(rptr, tm.tm_year, s_2digit, 2);
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_hour, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, tm.tm_min, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, tm.tm_sec, s_2digit, 2);
    do_match(rptr, " GMT", 4);

    // Accept nothing if any of the operations above has failed.
    if(rptr == nullptr)
      return 0;

    // Compose the timestamp and return the number of characters
    // that have been accepted.
    this->m_tp = (unix_time)(seconds) ::timegm(&tm);
    return (size_t) (rptr - str);
  }

size_t
HTTP_DateTime::
parse_rfc850_partial(const char* str)
  {
    const char* rptr = str;
    ::tm tm = { };

    // `Sunday, 06-Nov-94 08:49:37 GMT`
    do_match(rptr, tm.tm_wday, s_weekday, -1);
    do_match(rptr, ", ", 2);
    do_match(rptr, tm.tm_mday, s_2digit, 2);
    do_match(rptr, "-", 1);
    do_match(rptr, tm.tm_mon, s_month, 3);
    do_match(rptr, "-", 1);
    do_match(rptr, tm.tm_year, s_2digit, 2);
    tm.tm_year += ((tm.tm_year - 70) >> 15) & 100;
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_hour, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, tm.tm_min, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, tm.tm_sec, s_2digit, 2);
    do_match(rptr, " GMT", 4);

    // Accept nothing if any of the operations above has failed.
    if(rptr == nullptr)
      return 0;

    // Compose the timestamp and return the number of characters
    // that have been accepted.
    this->m_tp = (unix_time)(seconds) ::timegm(&tm);
    return (size_t) (rptr - str);
  }

size_t
HTTP_DateTime::
parse_asctime_partial(const char* str)
  {
    const char* rptr = str;
    ::tm tm = { };

    // `Sun Nov  6 08:49:37 1994`
    do_match(rptr, tm.tm_wday, s_weekday, 3);
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_mon, s_month, 3);
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_mday, s_sp1digit, 2);
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_hour, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, tm.tm_min, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, tm.tm_sec, s_2digit, 2);
    do_match(rptr, " ", 1);
    do_match(rptr, tm.tm_year, s_2digit, 2);
    tm.tm_year = tm.tm_year * 100 - 1900;
    do_match(rptr, tm.tm_year, s_2digit, 2);

    // Accept nothing if any of the operations above has failed.
    if(rptr == nullptr)
      return 0;

    // Compose the timestamp and return the number of characters
    // that have been accepted.
    this->m_tp = (unix_time)(seconds) ::timegm(&tm);
    return (size_t) (rptr - str);
  }

size_t
HTTP_DateTime::
parse(const char* str, size_t len)
  {
    // A string with an erroneous length will not be accepted, so
    // we just need to check for possibilities by `len`.
    size_t acc_len = 0;

    if(len >= 30) {
      acc_len = this->parse_rfc850_partial(str);  // 30

      if(acc_len == 0)
        acc_len = this->parse_rfc1123_partial(str);  // 29

      if(acc_len == 0)
        acc_len = this->parse_asctime_partial(str);  // 24
    }
    else if(len >= 29) {
      acc_len = this->parse_rfc1123_partial(str);  // 29

      if(acc_len == 0)
        acc_len = this->parse_asctime_partial(str);  // 24
    }
    else if(len >= 24)
      acc_len = this->parse_asctime_partial(str);  // 24

    return acc_len;
  }

size_t
HTTP_DateTime::
print_rfc1123_partial(char* str) const noexcept
  {
    char* wptr = str;
    ::time_t tp = this->m_tp.time_since_epoch().count();
    ::tm tm;
    ::gmtime_r(&tp, &tm);

    // `Sun, 06 Nov 1994 08:49:37 GMT`
    xmemrpcpy(wptr, s_weekday[(uint32_t) tm.tm_wday], 3);
    xstrrpcpy(wptr, ", ");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_mday], 2);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_month[(uint32_t) tm.tm_mon], 3);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_year / 100 + 19], 2);
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_year % 100], 2);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_hour], 2);
    xstrrpcpy(wptr, ":");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_min], 2);
    xstrrpcpy(wptr, ":");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_sec], 2);
    xstrrpcpy(wptr, " GMT");

    // Return the number of characters that have been written.
    return (size_t) (wptr - str);
  }

size_t
HTTP_DateTime::
print_rfc850_partial(char* str) const noexcept
  {
    char* wptr = str;
    ::time_t tp = this->m_tp.time_since_epoch().count();
    ::tm tm;
    ::gmtime_r(&tp, &tm);

    // `Sunday, 06-Nov-94 08:49:37 GMT`
    xstrrpcpy(wptr, s_weekday[(uint32_t) tm.tm_wday]);
    xstrrpcpy(wptr, ",");
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_mday], 2);
    xstrrpcpy(wptr, "-");
    xmemrpcpy(wptr, s_month[(uint32_t) tm.tm_mon], 3);
    xstrrpcpy(wptr, "-");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_year % 100], 2);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_hour], 2);
    xstrrpcpy(wptr, ":");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_min], 2);
    xstrrpcpy(wptr, ":");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_sec], 2);
    xstrrpcpy(wptr, " GMT");

    // Return the number of characters that have been written.
    return (size_t) (wptr - str);
  }

size_t
HTTP_DateTime::
print_asctime_partial(char* str) const noexcept
  {
    char* wptr = str;
    ::time_t tp = this->m_tp.time_since_epoch().count();
    ::tm tm;
    ::gmtime_r(&tp, &tm);

    // `Sun Nov  6 08:49:37 1994`
    xmemrpcpy(wptr, s_weekday[(uint32_t) tm.tm_wday], 3);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_month[(uint32_t) tm.tm_mon], 3);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_sp1digit[(uint32_t) tm.tm_mday], 2);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_hour], 2);
    xstrrpcpy(wptr, ":");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_min], 2);
    xstrrpcpy(wptr, ":");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_sec], 2);
    xstrrpcpy(wptr, " ");
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_year / 100 + 19], 2);
    xmemrpcpy(wptr, s_2digit[(uint32_t) tm.tm_year % 100], 2);
    xstrrpcpy(wptr, "");

    // Return the number of characters that have been written.
    return (size_t) (wptr - str);
  }

tinyfmt&
HTTP_DateTime::
print(tinyfmt& fmt) const
  {
    char str[64];
    size_t len = this->print_rfc1123_partial(str);
    return fmt.putn(str, len);
  }

cow_string
HTTP_DateTime::
print_to_string() const
  {
    char str[64];
    size_t len = this->print_rfc1123_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
