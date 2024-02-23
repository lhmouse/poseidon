// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_VALUE_
#define POSEIDON_HTTP_HTTP_VALUE_

#include "../fwd.hpp"
#include "../base/datetime.hpp"
namespace poseidon {

class HTTP_Value
  {
  private:
    ::rocket::variant<nullptr_t, cow_string, double, DateTime> m_stor;

  public:
    // Value constructors
    constexpr HTTP_Value(nullptr_t = nullptr) noexcept { }

    HTTP_Value&
    operator=(nullptr_t) & noexcept
      {
        this->m_stor.emplace<nullptr_t>();
        return *this;
      }

    HTTP_Value(cow_stringR str) noexcept
      {
        this->m_stor.emplace<cow_string>(str);
      }

    HTTP_Value&
    operator=(cow_stringR str) & noexcept
      {
        this->m_stor = str;
        return *this;
      }

    template<size_t N>
    HTTP_Value(const char (*ps)[N]) noexcept
      {
        this->m_stor.emplace<cow_string>(ps);
      }

    template<size_t N>
    HTTP_Value&
    operator=(const char (*ps)[N]) noexcept
      {
        this->mut_string() = ps;
        return *this;
      }

    HTTP_Value(double num) noexcept
      {
        this->m_stor.emplace<double>(num);
      }

    HTTP_Value&
    operator=(double num) & noexcept
      {
        this->m_stor = num;
        return *this;
      }

    HTTP_Value(int num) noexcept
      {
        this->m_stor.emplace<double>(num);
      }

    HTTP_Value&
    operator=(int num) & noexcept
      {
        this->m_stor = static_cast<double>(num);
        return *this;
      }

    HTTP_Value(const DateTime& dt) noexcept
      {
        this->m_stor.emplace<DateTime>(dt);
      }

    HTTP_Value&
    operator=(const DateTime& dt) & noexcept
      {
        this->m_stor = dt;
        return *this;
      }

    HTTP_Value(system_time tm) noexcept
      {
        this->m_stor.emplace<DateTime>(tm);
      }

    HTTP_Value&
    operator=(system_time tm) & noexcept
      {
        this->m_stor.emplace<DateTime>(tm);
        return *this;
      }

    HTTP_Value&
    swap(HTTP_Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    HTTP_Value(const HTTP_Value&) = default;
    HTTP_Value(HTTP_Value&&) = default;
    HTTP_Value& operator=(const HTTP_Value&) & = default;
    HTTP_Value& operator=(HTTP_Value&&) & = default;
    ~HTTP_Value();

    void
    clear() noexcept
      { this->m_stor.emplace<nullptr_t>();  }

    // Access raw data.
    bool
    is_null() const noexcept
      { return this->m_stor.ptr<nullptr_t>() != nullptr;  }

    bool
    is_string() const noexcept
      { return this->m_stor.ptr<cow_string>() != nullptr;  }

    cow_stringR
    as_string() const
      { return this->m_stor.as<cow_string>();  }

    const char*
    str_data() const
      { return this->m_stor.as<cow_string>().data();  }

    size_t
    str_size() const
      { return this->m_stor.as<cow_string>().size();  }

    cow_string&
    mut_string() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<cow_string>())
          return *ptr;
        else
          return this->m_stor.emplace<cow_string>();
      }

    bool
    is_number() const noexcept
      { return this->m_stor.ptr<double>() != nullptr;  }

    const double&
    as_number() const
      { return this->m_stor.as<double>();  }

    double&
    mut_number() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<double>())
          return *ptr;
        else
          return this->m_stor.emplace<double>();
      }

    bool
    is_datetime() const noexcept
      { return this->m_stor.ptr<DateTime>() != nullptr;  }

    const DateTime&
    as_datetime() const
      { return this->m_stor.as<DateTime>();  }

    system_time
    as_system_time() const
      { return this->m_stor.as<DateTime>().as_system_time();  }

    DateTime&
    mut_datetime() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<DateTime>())
          return *ptr;
        else
          return this->m_stor.emplace<DateTime>();
      }

    // Try parsing a quoted string. Upon success, the number of characters that
    // have been accepted is returned. Otherwise zero is returned, and the
    // contents of this object is indeterminate.
    size_t
    parse_quoted_string_partial(chars_view str);

    // Try parsing a floating-point number, starting with a decimal digit. Upon
    // success, the number of characters that have been accepted is returned.
    // Otherwise zero is returned, and the contents of this object are
    // indeterminate.
    size_t
    parse_number_partial(chars_view str);

    // Try parsing an HTTP date/time partially from a string. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero
    // is returned, and the contents of this object are indeterminate.
    size_t
    parse_datetime_partial(chars_view str);

    // Try parsing an HTTP token and store it as a string. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero
    // is returned, and the contents of this object are indeterminate.
    size_t
    parse_token_partial(chars_view str);

    // Try parsing an HTTP unquoted string and store it. This is a more
    // permissive variant of a token. All characters other than whitespace,
    // control characters, `=`, `,` and `;` are accepted. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero
    // is returned, and the contents of this object are indeterminate.
    size_t
    parse_unquoted_partial(chars_view str);

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
    parse(chars_view str);

    // Converts this value to its string form. The result will be suitable
    // for immediate use in an HTTP header. It is important to note that
    // HTTP date/time, which contain a comma itself, will not be enclosed
    // in double quotes.
    tinyfmt&
    print_to(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

inline
void
swap(HTTP_Value& lhs, HTTP_Value& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const HTTP_Value& value)
  { return value.print_to(fmt);  }

}  // namespace poseidon
#endif
