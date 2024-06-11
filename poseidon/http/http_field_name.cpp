// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_field_name.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct ci_reader_str
  {
    const char* cur;
    const char* end;
    int ch;

    explicit
    ci_reader_str(const cow_string& str) noexcept
      {
        this->cur = str.data();
        this->end = str.data() + str.size();
      }

    int
    get() noexcept
      {
        if(this->cur == this->end)
          this->ch = -1;
        else {
          this->ch = (unsigned char) *(this->cur);
          this->ch |= (('A' - this->ch - 1) & (this->ch - 'Z' - 1) & 0x100) >> 3;
          this->cur ++;
        }
        return this->ch;
      }
  };

struct ci_reader_sz
  {
    const char* cur;
    int ch;

    explicit
    ci_reader_sz(const char* str) noexcept
      {
        this->cur = str;
      }

    int
    get() noexcept
      {
        this->ch = (unsigned char) *(this->cur);
        if(this->ch == 0)
          this->ch = -1;
        else {
          this->ch |= (('A' - this->ch - 1) & (this->ch - 'Z' - 1) & 0x100) >> 3;
          this->cur ++;
        }
        return this->ch;
      }
  };

}  // namespace

HTTP_Field_Name::
~HTTP_Field_Name()
  {
  }

int
HTTP_Field_Name::
compare(const char* cmps) const noexcept
  {
    ci_reader_str rs1(this->m_str);
    ci_reader_sz rs2(cmps);
    while((rs1.get() == rs2.get()) && (rs1.ch != -1));
    return rs1.ch - rs2.ch;
  }

int
HTTP_Field_Name::
compare(cow_stringR cmps) const noexcept
  {
    ci_reader_str rs1(this->m_str);
    ci_reader_str rs2(cmps);
    while((rs1.get() == rs2.get()) && (rs1.ch != -1));
    return rs1.ch - rs2.ch;
  }

int
HTTP_Field_Name::
compare(const HTTP_Field_Name& other) const noexcept
  {
    ci_reader_str rs1(this->m_str);
    ci_reader_str rs2(other.m_str);
    while((rs1.get() == rs2.get()) && (rs1.ch != -1));
    return rs1.ch - rs2.ch;
  }

size_t
HTTP_Field_Name::
rdhash() const noexcept
  {
    // https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function#FNV-1a_hash
    ci_reader_str rs1(this->m_str);
    uint32_t hval = 2166136261;
    while(rs1.get() != -1) hval = (hval ^ (uint32_t) rs1.ch) * 16777619;
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
          this->m_str.mut(k) = (char) (this->m_str[k] | 0x20);
          break;

        default:
          POSEIDON_THROW(("Invalid HTTP field name `$1`"), this->m_str);
        }
  }

}  // namespace poseidon
