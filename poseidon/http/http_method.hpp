// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_METHOD_
#define POSEIDON_HTTP_HTTP_METHOD_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

class HTTP_Method
  {
  private:
    uint64_t m_number;

  public:
    // Constructs a method from a pre-computed enumerator. The conversion should
    // be implicit.
    constexpr HTTP_Method(HTTP_Method_Number number = http_method_null) noexcept
      :
        m_number(number)
      { }

    HTTP_Method&
    operator=(HTTP_Method_Number number) & noexcept
      {
        this->m_number = number;
        return *this;
      }

    // Constructs a method from a string, like `parse()`.
    // An exception is thrown if the method string is invalid or too long.
    HTTP_Method(chars_view str);

    HTTP_Method&
    swap(HTTP_Method& other) noexcept
      {
        ::std::swap(this->m_number, other.m_number);
        return *this;
      }

  public:
    // Gets the enumerator (of enum `HTTP_Method_Number`) of the current method,
    // eligible in a `switch` statement.
    constexpr
    uint64_t
    as_number() const noexcept
      { return this->m_number;  }

    // Tries parsing a method string and returns the number of characters that
    // have been parsed successfully.
    size_t
    parse(chars_view str) noexcept;

    // Converts this method to its string representation. The caller should
    // supply a buffer for 14 characters. A null terminator is always appended.
    // The number of characters that have been written is returned.
    size_t
    print_partial(char* str) const noexcept;

    tinyfmt&
    print_to(tinyfmt& fmt) const;

    cow_string
    to_string() const;
  };

inline
void
swap(HTTP_Method& lhs, HTTP_Method& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const HTTP_Method& value)
  { return value.print_to(fmt);  }

inline
bool
operator==(const HTTP_Method& lhs, const HTTP_Method& rhs) noexcept
  { return lhs.as_number() == rhs.as_number();  }

inline
bool
operator==(const HTTP_Method& lhs, HTTP_Method_Number rhs) noexcept
  { return lhs.as_number() == rhs;  }

inline
bool
operator==(HTTP_Method_Number lhs, const HTTP_Method& rhs) noexcept
  { return lhs == rhs.as_number();  }

inline
bool
operator!=(const HTTP_Method& lhs, const HTTP_Method& rhs) noexcept
  { return lhs.as_number() != rhs.as_number();  }

inline
bool
operator!=(const HTTP_Method& lhs, HTTP_Method_Number rhs) noexcept
  { return lhs.as_number() != rhs;  }

inline
bool
operator!=(HTTP_Method_Number lhs, const HTTP_Method& rhs) noexcept
  { return lhs != rhs.as_number();  }

}  // namespace poseidon
#endif
