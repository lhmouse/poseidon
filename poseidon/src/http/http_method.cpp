// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/http_method.hpp"
#include "../../utils.hpp"
namespace poseidon {

HTTP_Method::
HTTP_Method(chars_view str)
  {
    if(this->parse(str) != str.n)
      POSEIDON_THROW(("Failed to parse HTTP method string `$1`"), str);
  }

size_t
HTTP_Method::
parse(chars_view str) noexcept
  {
    if(str.n > 13)
      return 0;

    // Encode the method with base28.
    uint64_t reg = 0;
    uint32_t pos = static_cast<uint32_t>(str.n) - 1;
    while(pos != UINT32_MAX) {
      uint32_t ch = static_cast<uint8_t>(str.p[pos --]);
      reg *= 28;
      if((ch >= 'A') && (ch <= 'Z'))
        reg += ch - 'A' + 1;
      else if(ch == '-')
        reg += 27;
      else if(ch == '_')
        reg += 28;
      else
        return 0;
    }

    this->m_number = reg;
    return str.n;
  }

size_t
HTTP_Method::
print_partial(char* str) const noexcept
  {
    char* wptr = str;
    uint64_t reg = this->m_number;
    while(reg != 0) {
      uint64_t next_reg = (reg - 1) / 28;
      uint32_t digit = static_cast<uint32_t>(reg - next_reg * 28);
      reg = next_reg;
      if(digit <= 26)
        *(wptr ++) = static_cast<char>('A' + digit - 1);
      else if(digit == 27)
        *(wptr ++) = '-';
      else if(digit == 28)
        *(wptr ++) = '_';
    }

    *wptr = 0;
    return static_cast<size_t>(wptr - str);
  }

tinyfmt&
HTTP_Method::
print_to(tinyfmt& fmt) const
  {
    char str[16];
    size_t len = this->print_partial(str);
    return fmt.putn(str, len);
  }

cow_string
HTTP_Method::
to_string() const
  {
    char str[16];
    size_t len = this->print_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
