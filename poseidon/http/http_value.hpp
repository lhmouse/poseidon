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
    constexpr HTTP_Value(nullptr_t = nullptr) noexcept
      :
        m_stor()
      { }

    HTTP_Value(cow_stringR str) noexcept
      :
        m_stor(str)
      { }

    HTTP_Value(double num) noexcept
      :
        m_stor(num)
      { }

    HTTP_Value(int num) noexcept
      :
        m_stor(static_cast<double>(num))
      { }

    HTTP_Value(const DateTime& dt) noexcept
      :
        m_stor(dt)
      { }

    HTTP_Value(system_time tm) noexcept
      :
        m_stor(DateTime(time_point_cast<seconds>(tm)))
      { }

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
    operator=(double num) & noexcept
      {
        this->set_number(num);
        return *this;
      }

    HTTP_Value&
    operator=(int num) & noexcept
      {
        this->set_number(num);
        return *this;
      }

    HTTP_Value&
    operator=(const DateTime& dt) & noexcept
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
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    HTTP_Value(const HTTP_Value&) = default;
    HTTP_Value(HTTP_Value&&) = default;
    HTTP_Value& operator=(const HTTP_Value&) & = default;
    HTTP_Value& operator=(HTTP_Value&&) & = default;
    ~HTTP_Value();

    // Access raw data.
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

    const char*
    str_data() const
      { return this->m_stor.as<cow_string>().c_str();  }

    size_t
    str_length() const
      { return this->m_stor.as<cow_string>().length();  }

    cow_string&
    mut_string()
      { return this->m_stor.mut<cow_string>();  }

    void
    set_string(cow_stringR str) noexcept
      { this->m_stor = str;  }

    void
    set_string(const char* str, size_t len)
      {
        if(auto ps = this->m_stor.mut_ptr<cow_string>())
          ps->assign(str, len);
        else
          this->m_stor.emplace<cow_string>(str, len);
      }

    bool
    is_number() const noexcept
      { return this->m_stor.ptr<double>() != nullptr;  }

    const double&
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
      { return this->m_stor.ptr<DateTime>() != nullptr;  }

    const DateTime&
    as_datetime() const
      { return this->m_stor.as<DateTime>();  }

    time_t
    as_time_t() const
      { return this->m_stor.as<DateTime>().as_time_t();  }

    DateTime&
    mut_datetime()
      { return this->m_stor.mut<DateTime>();  }

    void
    set_datetime(const DateTime& dt) noexcept
      { this->m_stor = dt;  }

    void
    set_time_t(time_t dt) noexcept
      { this->m_stor.emplace<DateTime>().set_time_t(dt);  }

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
    print(tinyfmt& fmt) const;

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
  { return value.print(fmt);  }

}  // namespace poseidon
#endif
