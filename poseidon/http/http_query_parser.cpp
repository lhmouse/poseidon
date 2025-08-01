// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_query_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Query_Parser::
HTTP_Query_Parser()
  noexcept
  {
  }

HTTP_Query_Parser::
~HTTP_Query_Parser()
  {
  }

void
HTTP_Query_Parser::
clear()
  noexcept
  {
    this->m_hstr.clear();
    this->m_hpos = 0;

    this->m_name.clear();
    this->m_value.clear();
  }

void
HTTP_Query_Parser::
reload(const cow_string& raw_query)
  {
    this->m_hstr = raw_query;
    this->m_hpos = SIZE_MAX;

    this->m_name.clear();
    this->m_value.clear();
  }

bool
HTTP_Query_Parser::
next_element()
  {
    // The first call shall retrieve the first name-value pair. If `m_hpos` equals
    // `m_hstr.size()` then the end of the input string will have been reached;
    // otherwise it indicates an error, so don't touch it.
    if((this->m_hpos != SIZE_MAX) && (this->m_hpos >= this->m_hstr.size()))
      return false;

    // Skip the current separator.
    this->m_hpos ++;
    ROCKET_ASSERT(this->m_hpos <= this->m_hstr.size());
    const char* sptr = this->m_hstr.c_str() + this->m_hpos;
    const char* const esptr = this->m_hstr.c_str() + this->m_hstr.size();

    // Each ASCII character that is allowed in a query string has its
    // corresponding bit set to zero.
    // Reference: https://www.rfc-editor.org/rfc/rfc3986#section-3.4
    static constexpr uint32_t bad_chars[4] = { 0xFFFFFFFF, 0x5000000D, 0x78000000, 0xB8000001 };

    // Get a name-value pair. If a plain equals sign is encountered, it shall
    // separate the value from the name。
    this->m_name.clear();
    this->m_value.clear();
    size_t name_len = SIZE_MAX;

    while(sptr != esptr) {
      uint32_t ch = (uint8_t) *sptr;
      if(ch < 128) {
        if((bad_chars[ch / 32] >> (ch % 32)) & 1) {
          POSEIDON_LOG_DEBUG(("Invalid character encountered at `$1`"), snview(sptr, 40));
          this->m_hpos = error_hpos;
          return false;
        }

        if(ch == '&') {
          // This name-value pair ends here.
          break;
        }
        else if(ch == '%') {
          // Expect two hexadecimal digits.
          if(esptr - sptr < 3) {
            POSEIDON_LOG_DEBUG(("Invalid % sequence encountered at `$1`"), snview(sptr, 40));
            this->m_hpos = error_hpos;
            return false;
          }

          if((sptr[1] >= '0') && (sptr[1] <= '9'))
            ch = (uint32_t) ((uint8_t) sptr[1] - '0') << 4;
          else if((sptr[1] >= 'A') && (sptr[1] <= 'F'))
            ch = (uint32_t) ((uint8_t) sptr[1] - 'A' + 10) << 4;
          else if((sptr[1] >= 'a') && (sptr[1] <= 'f'))
            ch = (uint32_t) ((uint8_t) sptr[1] - 'a' + 10) << 4;
          else {
            POSEIDON_LOG_DEBUG(("Invalid hex digit encountered at `$1`"), snview(sptr, 40));
            this->m_hpos = error_hpos;
            return false;
          }

          if((sptr[2] >= '0') && (sptr[2] <= '9'))
            ch |= (uint32_t) ((uint8_t) sptr[2] - '0');
          else if((sptr[2] >= 'A') && (sptr[2] <= 'F'))
            ch |= (uint32_t) ((uint8_t) sptr[2] - 'A' + 10);
          else if((sptr[2] >= 'a') && (sptr[2] <= 'f'))
            ch |= (uint32_t) ((uint8_t) sptr[2] - 'a' + 10);
          else {
            POSEIDON_LOG_DEBUG(("Invalid hex digit encountered at `$1`"), snview(sptr, 40));
            this->m_hpos = error_hpos;
            return false;
          }

          // Accept this sequence.
          this->m_name.mut_str() += (char) ch;
          sptr += 3;
          continue;
        }
        else if(ch == '+') {
          // Accept a space.
          this->m_name.mut_str() += ' ';
          sptr ++;
          continue;
        }
        else if((ch == '=') && (name_len == SIZE_MAX)) {
          // Record this position as the beginning of the value. The equals
          // sign itself is not stored.
          name_len = this->m_name.length();
          sptr ++;
          continue;
        }
      }

      // Accept this character verbatim.
      this->m_name.mut_str() += (char) ch;
      sptr ++;
    }

    if(name_len != SIZE_MAX) {
      // Split the string.
      this->m_value.set(this->m_name.c_str() + name_len, this->m_name.size() - name_len);
      this->m_name.mut_str().erase(name_len);
    }

    // Accept this name-value pair.
    this->m_hpos = (size_t) (sptr - this->m_hstr.c_str());
    return true;
  }

}  // namespace poseidon
