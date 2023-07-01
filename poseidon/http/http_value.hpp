// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_VALUE_
#define POSEIDON_HTTP_HTTP_VALUE_

#include "../fwd.hpp"
#include "http_datetime.hpp"
namespace poseidon {

class HTTP_Value
  {
  private:
    variant<nullptr_t, cow_string, double, HTTP_DateTime> m_stor;

  public:
    // Value constructors
    constexpr
    HTTP_Value(nullptr_t = nullptr) noexcept
      : m_stor()
      { }

    HTTP_Value(cow_stringR str) noexcept
      : m_stor(str)
      { }

    HTTP_Value(int num) noexcept
      : m_stor((double) num)
      { }

    HTTP_Value(long num) noexcept
      : m_stor((double) num)
      { }

    HTTP_Value(long long num) noexcept
      : m_stor((double) num)
      { }

    HTTP_Value(double num) noexcept
      : m_stor(num)
      { }

    HTTP_Value(const HTTP_DateTime& tm) noexcept
      : m_stor(tm)
      { }

    HTTP_Value(system_time tm) noexcept
      : m_stor((HTTP_DateTime) time_point_cast<seconds>(tm))
      { }

    HTTP_Value&
    operator=(nullptr_t) & noexcept
      {
        this->m_stor = nullptr;
        return *this;
      }

    HTTP_Value&
    operator=(cow_stringR str) & noexcept
      {
        this->m_stor = str;
        return *this;
      }

    HTTP_Value&
    operator=(int num) & noexcept
      {
        this->m_stor = (double) num;
        return *this;
      }

    HTTP_Value&
    operator=(long num) & noexcept
      {
        this->m_stor = (double) num;
        return *this;
      }

    HTTP_Value&
    operator=(long long num) & noexcept
      {
        this->m_stor = (double) num;
        return *this;
      }

    HTTP_Value&
    operator=(double num) & noexcept
      {
        this->m_stor = num;
        return *this;
      }

    HTTP_Value&
    operator=(const HTTP_DateTime& tm) & noexcept
      {
        this->m_stor = tm;
        return *this;
      }

    HTTP_Value&
    operator=(system_time tm) & noexcept
      {
        this->m_stor = (HTTP_DateTime) time_point_cast<seconds>(tm);
        return *this;
      }

    HTTP_Value&
    swap(HTTP_Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(HTTP_Value);

    // Accesses raw data.
    bool
    is_null() const noexcept
      { return this->m_stor.ptr<nullptr_t>() != nullptr;  }

    void
    clear() noexcept
      { this->m_stor = nullptr;  }

    bool
    is_string() const noexcept
      { return this->m_stor.ptr<cow_string>() != nullptr;  }

    cow_stringR
    as_string() const
      { return this->m_stor.as<cow_string>();  }

    cow_string&
    mut_string()
      { return this->m_stor.mut<cow_string>();  }

    void
    set_string(cow_stringR str) noexcept
      { this->m_stor = str;  }

    bool
    is_number() const noexcept
      { return this->m_stor.ptr<double>() != nullptr;  }

    double
    as_number() const
      { return this->m_stor.as<double>();  }

    double&
    mut_number()
      { return this->m_stor.mut<double>();  }

    void
    set_number(double num) noexcept
      { this->m_stor = num;  }

    bool
    is_datetime() const noexcept
      { return this->m_stor.ptr<HTTP_DateTime>() != nullptr;  }

    const HTTP_DateTime&
    as_datetime() const
      { return this->m_stor.as<HTTP_DateTime>();  }

    HTTP_DateTime&
    mut_datetime()
      { return this->m_stor.mut<HTTP_DateTime>();  }

    void
    set_datetime(const HTTP_DateTime& tm) noexcept
      { this->m_stor = tm;  }

    // Try parsing a quoted string. Upon success, the number of characters that
    // have been accepted is returned. Otherwise zero is returned, and the
    // contents of this object is indeterminate.
    size_t
    parse_quoted_string_partial(const char* str, size_t len);

    // Try parsing a floating-point number, starting with a decimal digit. Upon
    // success, the number of characters that have been accepted is returned.
    // Otherwise zero is returned, and the contents of this object are
    // indeterminate.
    size_t
    parse_number_partial(const char* str, size_t len);

    // Try parsing an HTTP date/time partially from a string. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero
    // is returned, and the contents of this object are indeterminate.
    size_t
    parse_datetime_partial(const char* str, size_t len);

    // Try parsing an HTTP token and store it as a string. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero
    // is returned, and the contents of this object are indeterminate.
    size_t
    parse_token_partial(const char* str, size_t len);

    // Try parsing an HTTP unquoted string and store it. This is a more
    // permissive variant of a token. All characters other than whitespace,
    // control characters, `=`, `,` and `;` are accepted. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero
    // is returned, and the contents of this object are indeterminate.
    size_t
    parse_unquoted_partial(const char* str, size_t len);

    // Try parsing an HTTP value, possibly from an HTTP header. The string is
    // matched against these rules (in this order):
    //
    // * a quoted string, enclosed in a pair of double quotes
    // * a floating-point number, starting with a digit
    // * an HTTP date/time, starting with a weekday
    // * an HTTP unquoted string
    //
    // If a match is found and parsed accordingly, the number of characters
    // that have been consumed is returned. Otherwise zero is  returned, and
    // the contents of this object are indeterminate.
    size_t
    parse(const char* str, size_t len);

    // Converts this value to its string form. The result will be suitable
    // for immediate use in an HTTP header. It is important to note that
    // HTTP date/time, which contain a comma itself, will not be enclosed
    // in double quotes.
    tinyfmt&
    print(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

inline
void
swap(HTTP_Value& lhs, HTTP_Value& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const HTTP_Value& value)
  {
    return value.print(fmt);
  }

}  // namespace poseidon
#endif
