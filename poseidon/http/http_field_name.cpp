// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_field_name.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Field_Name::
~HTTP_Field_Name()
  {
  }

int
HTTP_Field_Name::
compare(const cow_string& cmps)
  const noexcept
  {
    return ::rocket::ascii_ci_compare(this->m_str.data(), this->m_str.size(),
                                      cmps.data(), cmps.size());
  }

int
HTTP_Field_Name::
compare(const HTTP_Field_Name& other)
  const noexcept
  {
    return ::rocket::ascii_ci_compare(this->m_str.data(), this->m_str.size(),
                                      other.m_str.data(), other.m_str.size());
  }

size_t
HTTP_Field_Name::
rdhash()
  const noexcept
  {
    return ::rocket::ascii_ci_hash(this->m_str.data(), this->m_str.size());
  }

void
HTTP_Field_Name::
canonicalize()
  {
    for(size_t k = 0;  k != this->m_str.size();  ++k)
      switch(this->m_str[k])
        {
        case 'a' ... 'z':
        case '0' ... '9':
        case '-':
        case '_':
          break;

        case 'A' ... 'Z':
          // Convert this letter to lowercase.
          this->m_str.mut(k) = (char) (this->m_str[k] | 0x20);
          break;

        default:
          POSEIDON_THROW(("Invalid HTTP field name `$1`"), this->m_str);
        }
  }

}  // namespace poseidon
