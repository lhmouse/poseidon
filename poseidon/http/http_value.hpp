// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_VALUE_
#define POSEIDON_HTTP_HTTP_VALUE_

#include "../fwd.hpp"
#include "../base/datetime.hpp"
namespace poseidon {

class HTTP_Value
  {
  private:
    enum : uint8_t
      {
        vm_str_valid  = 0b00000001,
        vm_int_valid  = 0b00000010,
        vm_dbl_valid  = 0b00000100,
        vm_dt_valid   = 0b00001000,
      };

    cow_string m_str;
    int64_t m_int = 0;
    double m_dbl = 0;
    DateTime m_dt;
    uint8_t m_vm = 0;

  private:
    void
    do_update_variants();

  public:
    // Initialize a value from the given argument. It's important that all fields
    // are updated accordingly, so users may read this value in any format.
    HTTP_Value()
      noexcept
      {
      }

    HTTP_Value(const cow_string& str)
      :
        m_str(str),
        m_vm(vm_str_valid)
      {
        this->do_update_variants();
      }

    HTTP_Value&
    operator=(const cow_string& str)
      &
      {
        this->m_str = str;
        this->m_vm = vm_str_valid;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value&
    operator=(cow_string&& str)
      &
      {
        this->m_str = move(str);
        this->m_vm = vm_str_valid;
        this->do_update_variants();
        return *this;
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    HTTP_Value(const ycharT (*ps)[N])
      :
        m_str(ps),
        m_vm(vm_str_valid)
      {
        this->do_update_variants();
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    HTTP_Value&
    operator=(const ycharT (*ps)[N])
      &
      {
        this->m_str = ps;
        this->m_vm = vm_str_valid;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value(int num)
      :
        m_int(num),
        m_vm(vm_int_valid)
      {
        this->do_update_variants();
      }

    HTTP_Value&
    operator=(int num)
      &
      {
        this->m_int = num;
        this->m_vm = vm_int_valid;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value(long num)
      :
        m_int(num),
        m_vm(vm_int_valid)
      {
        this->do_update_variants();
      }

    HTTP_Value&
    operator=(long num)
      &
      {
        this->m_int = num;
        this->m_vm = vm_int_valid;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value(long long num)
      :
        m_int(num),
        m_vm(vm_int_valid)
      {
        this->do_update_variants();
      }

    HTTP_Value&
    operator=(long long num)
      &
      {
        this->m_int = num;
        this->m_vm = vm_int_valid;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value(double num)
      :
        m_dbl(num),
        m_vm(vm_dbl_valid)
      {
        this->do_update_variants();
      }

    HTTP_Value&
    operator=(double num)
      &
      {
        this->m_dbl = num;
        this->m_vm = vm_dbl_valid;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value(const DateTime& dt)
      :
        m_dt(dt),
        m_vm(vm_dt_valid)
      {
        this->do_update_variants();
      }

    HTTP_Value&
    operator=(const DateTime& dt)
      &
      {
        this->m_vm = vm_dt_valid;
        this->m_dt = dt;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value(const system_time& tm)
      :
        m_dt(tm),
        m_vm(vm_dt_valid)
      {
        this->do_update_variants();
      }

    HTTP_Value&
    operator=(const system_time& tm)
      &
      {
        this->m_dt = tm;
        this->m_vm = vm_dt_valid;
        this->do_update_variants();
        return *this;
      }

    HTTP_Value&
    swap(HTTP_Value& other)
      noexcept
      {
        ::std::swap(this->m_int, other.m_int);
        this->m_str.swap(other.m_str);
        ::std::swap(this->m_dbl, other.m_dbl);
        this->m_dt.swap(other.m_dt);
        ::std::swap(this->m_vm, other.m_vm);
        return *this;
      }

  public:
    HTTP_Value(const HTTP_Value&) = default;
    HTTP_Value(HTTP_Value&&) = default;
    HTTP_Value& operator=(const HTTP_Value&) & = default;
    HTTP_Value& operator=(HTTP_Value&&) & = default;
    ~HTTP_Value();

    // Sets this value to an empty string.
    void
    clear()
      noexcept
      {
        this->m_int = 0;
        this->m_str.clear();
        this->m_dbl = 0;
        this->m_dt = system_time();
        this->m_vm = 0;
      }

    // Access individual fields. These are always synchronized.
    bool
    is_null()
      const noexcept
      { return !(this->m_vm & vm_str_valid);  }

    const cow_string&
    as_string()
      const noexcept
      { return this->m_str;  };

    const char*
    as_string_c_str()
      const noexcept
      { return this->m_str.c_str();  }

    size_t
    as_string_length()
      const noexcept
      { return this->m_str.length();  }

    bool
    is_integer()
      const noexcept
      { return this->m_vm & vm_int_valid;  }

    int64_t
    as_integer()
      const noexcept
      { return this->m_int;  }

    bool
    is_double()
      const noexcept
      { return this->m_vm & vm_dbl_valid;  }

    double
    as_double()
      const noexcept
      { return this->m_dbl;  }

    bool
    is_datetime()
      const noexcept
      { return this->m_vm & vm_dt_valid;  }

    const DateTime&
    as_datetime()
      const noexcept
      { return this->m_dt;  }

    system_time
    as_system_time()
      const noexcept
      { return this->m_dt.as_system_time();  }

    // Sets raw data.
    void
    set(const char* str, size_t len)
      {
        this->m_vm = vm_str_valid;
        this->m_str.assign(str, len);
        this->do_update_variants();
      }

    // Tries parsing a quoted string. Upon success, the number of characters that
    // have been accepted is returned. Otherwise zero is returned, and the contents
    // of this object is indeterminate.
    size_t
    parse_quoted_string_partial(chars_view str);

    // Tries parsing an HTTP date/time partially from a string. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero is
    // returned, and the contents of this object are indeterminate.
    size_t
    parse_datetime_partial(chars_view str);

    // Tries parsing an HTTP token and store it as a string. Upon success, the
    // number of characters that have been accepted is returned. Otherwise zero is
    // returned, and the contents of this object are indeterminate.
    size_t
    parse_token_partial(chars_view str);

    // Tries parsing an HTTP unquoted string and store it. This is a more permissive
    // variant of a token. All characters other than whitespace, control characters,
    // `=`, `,` and `;` are accepted. Upon success, the number of characters that
    // have been accepted is returned. Otherwise zero is returned, and the contents
    // of this object are indeterminate.
    size_t
    parse_unquoted_partial(chars_view str);

    // Tries parsing an HTTP value, possibly from an HTTP header. The string is
    // matched against these rules (in this order):
    //
    // * a quoted string, enclosed in a pair of double quotes
    // * a floating-point number, starting with a digit
    // * an HTTP date/time, starting with a weekday
    // * an HTTP unquoted string
    //
    // If a match is found and parsed accordingly, the number of characters that have
    // been consumed is returned. Otherwise zero is  returned, and the contents of
    // this object are indeterminate.
    size_t
    parse(chars_view str);

    // Converts this value to its string form. The result will be suitable for
    // immediate use in an HTTP header. It is important to note that HTTP date/time,
    // which contain a comma itself, will not be enclosed in double quotes.
    tinyfmt&
    print_to(tinyfmt& fmt)
      const;

    cow_string
    to_string()
      const;
  };

inline
void
swap(HTTP_Value& lhs, HTTP_Value& rhs)
  noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const HTTP_Value& value)
  { return value.print_to(fmt);  }

}  // namespace poseidon
#endif
