// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

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
compare(const char* cmps) const noexcept
  {
    for(size_t k = 0;; ++k) {
      int c1 = -1, c2 = -1;
      if(k != this->m_str.size())
        c1 = static_cast<uint8_t>(::rocket::ascii_to_lower(this->m_str[k]));
      if(cmps[k] != 0)
        c2 = static_cast<uint8_t>(::rocket::ascii_to_lower(cmps[k]));
      if(c1 != c2)
        return c1 - c2;
      if(c1 == -1)
        return 0;
    }
  }

int
HTTP_Field_Name::
compare(cow_stringR cmps) const noexcept
  {
    for(size_t k = 0;; ++k) {
      int c1 = -1, c2 = -1;
      if(k != this->m_str.size())
        c1 = static_cast<uint8_t>(::rocket::ascii_to_lower(this->m_str[k]));
      if(k != cmps.size())
        c2 = static_cast<uint8_t>(::rocket::ascii_to_lower(cmps[k]));
      if(c1 != c2)
        return c1 - c2;
      if(c1 == -1)
        return 0;
    }
  }

int
HTTP_Field_Name::
compare(const HTTP_Field_Name& other) const noexcept
  {
    return this->compare(other.m_str);
  }

size_t
HTTP_Field_Name::
rdhash() const noexcept
  {
    // https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function#FNV-1a_hash
    uint32_t hval = 2166136261;
    for(size_t k = 0;  k != this->m_str.size();  ++k)
      hval = (hval ^ static_cast<uint8_t>(::rocket::ascii_to_lower(this->m_str[k])))
             * 16777619;
    return hval;
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
          this->m_str.mut(k) = ::rocket::ascii_to_lower(this->m_str[k]);
          break;

        default:
          POSEIDON_THROW(("Invalid HTTP field name `$1`"), this->m_str);
        }
  }

}  // namespace poseidon
