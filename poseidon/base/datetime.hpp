// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_DATETIME_
#define POSEIDON_BASE_DATETIME_

#include "../fwd.hpp"
namespace poseidon {

class DateTime
  {
  private:
    system_time m_tp;

  public:
    // Initializes a timestamp of `1970-01-01 00:00:00 UTC`.
    constexpr
    DateTime() noexcept = default;

    constexpr
    DateTime(system_time tp) noexcept
      :
        m_tp(tp)
      { }

    DateTime&
    operator=(system_time tp) & noexcept
      {
        this->m_tp = tp;
        return *this;
      }

    DateTime&
    swap(DateTime& other) noexcept
      {
        ::std::swap(this->m_tp, other.m_tp);
        return *this;
      }

    // Parses a timestamp from an HTTP date/time string, like `parse()`.
    // An exception is thrown if the date/time string is not valid.
    explicit
    DateTime(chars_view str);

  public:
    // Accesses raw data.
    constexpr
    system_time
    as_system_time() const noexcept
      { return this->m_tp;  }

    void
    set_system_time(system_time tp) noexcept
      { this->m_tp = tp;  }

    // Tries parsing an HTTP date/time in the formal RFC 1123 format. An example
    // is `Sun, 06 Nov 1994 08:49:37 GMT`. This function returns the number of
    // characters that have been accepted, which is 29 upon success, and 0 upon
    // failure.
    size_t
    parse_rfc1123_partial(const char* str);

    // Tries parsing an HTTP date/time in the obsolete RFC 850 format. An example
    // is `Sunday, 06-Nov-94 08:49:37 GMT`. This function returns the number of
    // characters that have been accepted, which is within [30,33] upon success,
    // and 0 upon failure.
    size_t
    parse_rfc850_partial(const char* str);

    // Tries parsing an HTTP date/time in the obsolete asctime format. An example
    // is `Sun Nov  6 08:49:37 1994`. This function returns the number of
    // characters that have been accepted, which is 24 upon success, and 0 upon
    // failure.
    size_t
    parse_asctime_partial(const char* str);

    // Tries parsing an HTTP date/time in the cookie format. An example is `Sun,
    // 06-Nov-1994 08:49:37 GMT`. This function returns the number of characters
    // that have been accepted, which is 29 upon success, and 0 upon failure.
    size_t
    parse_cookie_partial(const char* str);

    // Tries parsing a general date/time in the ISO 8601 format. An example is
    // `1994-11-06T08:49:37Z`. Only the `Z` time zone specifier is allowed. This
    // function returns the number of characters that have been accepted, which
    // is 20 upon success, and 0 upon failure.
    size_t
    parse_iso8601_partial(const char* str);

    // Tries parsing an HTTP date/time in any of the formats above. If a date/time
    // string has been parsed, the number of characters that have been consumed
    // is returned. If zero is returned or an exception is thrown, the contents
    // of this object are unspecified.
    size_t
    parse(chars_view str);

    // Converts this timestamp to its RFC 1123 format, with a null terminator.
    // There shall be at least 30 characters in the buffer that `str` points to.
    // This function returns the number of characters that have been written,
    // excluding the null terminator, which is always 29.
    size_t
    print_rfc1123_partial(char* str) const noexcept;

    // Converts this timestamp to its RFC 1123 format, with a null terminator.
    // There shall be at least 34 characters in the buffer that `str` points to.
    // This function returns the number of characters that have been written,
    // excluding the null terminator, which is within [30,33].
    size_t
    print_rfc850_partial(char* str) const noexcept;

    // Converts this timestamp to its asctime format, with a null terminator.
    // There shall be at least 25 characters in the buffer that `str` points to.
    // This function returns the number of characters that have been written,
    // excluding the null terminator, which is always 24.
    size_t
    print_asctime_partial(char* str) const noexcept;

    // Converts this timestamp to its cookie format, with a null terminator.
    // There shall be at least 30 characters in the buffer that `str` points to.
    // This function returns the number of characters that have been written,
    // excluding the null terminator, which is always 29.
    size_t
    print_cookie_partial(char* str) const noexcept;

    // Converts this timestamp to its ISO 8601 format, with a null terminator.
    // There shall be at least 21 characters in the buffer that `str` points to.
    // This function returns the number of characters that have been written,
    // excluding the null terminator, which is always 20.
    size_t
    print_iso8601_partial(char* str) const noexcept;

    // Converts this timestamp to its ISO 8601 format with nanoseconds, with a
    // null terminator. There shall be at least 31 characters in the buffer
    // that `str` points to. This function returns the number of characters
    // that have been written, excluding the null terminator, which is always
    // 30.
    size_t
    print_iso8601_ns_partial(char* str) const noexcept;

    // Converts this timestamp to its ISO 8601 form.
    tinyfmt&
    print_to(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

constexpr
bool
operator==(const DateTime& lhs, const DateTime& rhs) noexcept
  { return lhs.as_system_time() == rhs.as_system_time();  }

constexpr
bool
operator!=(const DateTime& lhs, const DateTime& rhs) noexcept
  { return lhs.as_system_time() != rhs.as_system_time();  }

constexpr
bool
operator<(const DateTime& lhs, const DateTime& rhs) noexcept
  { return lhs.as_system_time() < rhs.as_system_time();  }

constexpr
bool
operator>(const DateTime& lhs, const DateTime& rhs) noexcept
  { return lhs.as_system_time() > rhs.as_system_time();  }

constexpr
bool
operator<=(const DateTime& lhs, const DateTime& rhs) noexcept
  { return lhs.as_system_time() <= rhs.as_system_time();  }

constexpr
bool
operator>=(const DateTime& lhs, const DateTime& rhs) noexcept
  { return lhs.as_system_time() >= rhs.as_system_time();  }

inline
void
swap(DateTime& lhs, DateTime& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const DateTime& ts)
  { return ts.print_to(fmt);  }

}  // namespace poseidon
#endif
