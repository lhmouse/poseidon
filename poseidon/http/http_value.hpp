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
    enum Index : uint8_t
      {
        index_null,
        index_string,
        index_number,
        index_datetime,
      };

    union {
      uintptr_t m_index_stor;
      Index m_index;
    };
    cow_string m_str;
    double m_num;
    HTTP_DateTime m_dt;

  private:
    inline
    void
    do_check_index(Index expect, const char* msg) const
      {
        if(this->m_index != expect)
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "HTTP_Value: value is not %s: index `%d` != `%d`",
              msg, this->m_index, expect);
      }

  public:
    // Value constructors
    constexpr
    HTTP_Value(nullptr_t = nullptr) noexcept
      : m_index_stor(), m_str(), m_num(), m_dt()  { }

    HTTP_Value(cow_stringR str) noexcept
      : m_index(index_string), m_str(str)  { }

    HTTP_Value(int num) noexcept
      : m_index(index_number), m_num(num)  { }

    HTTP_Value(double num) noexcept
      : m_index(index_number), m_num(num)  { }

    HTTP_Value(const HTTP_DateTime& dt) noexcept
      : m_index(index_datetime), m_dt(dt)  { }

    HTTP_Value(system_time tm) noexcept
      : m_index(index_datetime), m_dt(time_point_cast<seconds>(tm))  { }

    HTTP_Value&
    operator=(nullptr_t) & noexcept
      {
        this->clear();
        return *this;
      }

    HTTP_Value&
    operator=(cow_stringR str) & noexcept
      {
        this->set_string(str);
        return *this;
      }

    HTTP_Value&
    operator=(int num) & noexcept
      {
        this->set_number(num);
        return *this;
      }

    HTTP_Value&
    operator=(double num) & noexcept
      {
        this->set_number(num);
        return *this;
      }

    HTTP_Value&
    operator=(const HTTP_DateTime& dt) & noexcept
      {
        this->set_datetime(dt);
        return *this;
      }

    HTTP_Value&
    operator=(system_time tm) & noexcept
      {
        this->set_datetime(tm);
        return *this;
      }

    HTTP_Value&
    swap(HTTP_Value& other) noexcept
      {
        ::std::swap(this->m_index_stor, other.m_index_stor);
        this->m_str.swap(other.m_str);
        ::std::swap(this->m_num, other.m_num);
        this->m_dt.swap(other.m_dt);
        return *this;
      }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(HTTP_Value);

    // Accesses raw data.
    bool
    is_null() const noexcept
      { return this->m_index == index_null;  }

    void
    clear() noexcept
      { this->m_index = index_null;  }

    bool
    is_string() const noexcept
      { return this->m_index == index_string;  }

    cow_stringR
    as_string() const
      {
        this->do_check_index(index_string, "a string");
        return this->m_str;
      }

    cow_string&
    mut_string()
      {
        this->do_check_index(index_string, "a string");
        return this->m_str;
      }

    const char*
    as_c_str() const
      {
        this->do_check_index(index_string, "a string");
        return this->m_str.c_str();
      }

    char*
    mut_c_str()
      {
        this->do_check_index(index_string, "a string");
        return this->m_str.mut_data();
      }

    void
    set_string(cow_stringR str) noexcept
      {
        this->m_str = str;
        this->m_index = index_string;
      }

    void
    set_string(const char* str, size_t len) noexcept
      {
        this->m_str.assign(str, len);
        this->m_index = index_string;
      }

    bool
    is_number() const noexcept
      { return this->m_index == index_number;  }

    double
    as_number() const
      {
        this->do_check_index(index_number, "a number");
        return this->m_num;
      }

    double&
    mut_number()
      {
        this->do_check_index(index_number, "a number");
        return this->m_num;
      }

    void
    set_number(int num) noexcept
      {
        this->m_num = num;
        this->m_index = index_number;
      }

    void
    set_number(double num) noexcept
      {
        this->m_num = num;
        this->m_index = index_number;
      }

    bool
    is_datetime() const noexcept
      { return this->m_index == index_datetime;  }

    const HTTP_DateTime&
    as_datetime() const
      {
        this->do_check_index(index_datetime, "an HTTP date/time");
        return this->m_dt;
      }

    HTTP_DateTime&
    mut_datetime()
      {
        this->do_check_index(index_datetime, "an HTTP date/time");
        return this->m_dt;
      }

    void
    set_datetime(const HTTP_DateTime& dt) noexcept
      {
        this->m_dt = dt;
        this->m_index = index_datetime;
      }

    void
    set_datetime(system_time tm) noexcept
      {
        this->m_dt = time_point_cast<seconds>(tm);
        this->m_index = index_datetime;
      }

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
