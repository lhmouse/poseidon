// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_VALUE_
#define POSEIDON_HTTP_HTTP_VALUE_

#include "../fwd.hpp"
#include "http_datetime.hpp"
namespace poseidon {

class HTTP_Value
  {
  public:
    enum Index : uint8_t
      {
        index_null      = 0,
        index_string    = 1,
        index_number    = 2,
        index_datetime  = 3,
      };

  private:
    variant<
      ROCKET_CDR(
        ,nullptr_t      // index_null
        ,cow_string     // index_string
        ,double         // index_number
        ,HTTP_DateTime  // index_datetime
      )>
      m_stor;

  public:
    // Value constructors
    constexpr
    HTTP_Value(nullptr_t = nullptr) noexcept
      : m_stor() { }

    HTTP_Value(stringR str) noexcept
      : m_stor(str)  { }

    HTTP_Value(double num) noexcept
      : m_stor(num)  { }

    HTTP_Value(const HTTP_DateTime& tm) noexcept
      : m_stor(tm)  { }

    HTTP_Value&
    swap(HTTP_Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    // Accesses raw data.
    constexpr
    Index
    index() const noexcept
      { return (Index) this->m_stor.index();  }

    bool
    is_null() const noexcept
      { return this->m_stor.index() == index_null;  }

    void
    clear() noexcept
      { this->m_stor = nullptr;  }

    bool
    is_string() const noexcept
      { return this->m_stor.index() == index_string;  }

    const cow_string&
    as_string() const
      { return this->m_stor.as<index_string>();  }

    cow_string&
    mut_string()
      { return this->m_stor.mut<index_string>();  }

    void
    set_string(const cow_string& str) noexcept
      { this->m_stor = str;  }

    bool
    is_number() const noexcept
      { return this->m_stor.index() == index_number;  }

    double
    as_number() const
      { return this->m_stor.as<index_number>();  }

    double&
    mut_number()
      { return this->m_stor.mut<index_number>();  }

    void
    set_number(double num) noexcept
      { this->m_stor = num;  }

    bool
    is_datetime() const noexcept
      { return this->m_stor.index() == index_datetime;  }

    const HTTP_DateTime&
    as_datetime() const
      { return this->m_stor.as<index_datetime>();  }

    HTTP_DateTime&
    mut_datetime()
      { return this->m_stor.mut<index_datetime>();  }

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
    parse(stringR str);

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
operator<<(tinyfmt& fmt, const HTTP_Value& ts)
  {
    return ts.print(fmt);
  }

}  // namespace poseidon
#endif
