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

    // Try parsing an HTTP value, from a possible HTTP message header. The
    // string is matched against these rules (in this order):
    //
    // * an HTTP date/time, starting with a weekday
    // * a quoted string, enclosed in a pair of double quotes
    // * a floating-point number, starting with a digit
    // * an HTTP token
    //
    // If a match has been found and parsed accordingly, the number of
    // characters that have been consumed is returned. Otherwise zero is
    // returned, and the contents of this object is indeterminate.
    size_t
    parse(const char* str, size_t len);

    size_t
    parse(const char* str);

    size_t
    parse(cow_stringR str);

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
