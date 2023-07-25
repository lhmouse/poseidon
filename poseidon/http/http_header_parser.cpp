// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_header_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Header_Parser::
~HTTP_Header_Parser()
  {
  }

void
HTTP_Header_Parser::
reload(cow_stringR hstr)
  {
    this->m_hstr = hstr;
    this->m_hpos = SIZE_MAX;
    this->m_name.clear();
    this->m_value.clear();
  }

POSEIDON_VISIBILITY_HIDDEN
int
HTTP_Header_Parser::
do_next_attribute_from_separator()
  {
    // Skip the current separator.
    this->m_hpos ++;
    ROCKET_ASSERT(this->m_hpos <= this->m_hstr.size());
    const char* sptr = this->m_hstr.c_str() + this->m_hpos;

    // Skip whitespace and attribute separators. This function shall not move
    // across element boundaries.
    while((*sptr == ' ') || (*sptr == '\t') || (*sptr == ';'))
      sptr ++;

    if((sptr == this->m_hstr.c_str() + this->m_hstr.ssize()) || (*sptr == ','))
      return -1;

    // Parse the name of an attribute, and initialize its value to null.
    size_t tlen = this->m_value.parse_token_partial(sptr);
    if(tlen == 0) {
      this->m_hpos = error_hpos;
      return -1;
    }

    sptr += tlen;
    this->m_name.swap(this->m_value.mut_string());
    this->m_value = nullptr;

    // Skip bad whitespace.
    while((*sptr == ' ') || (*sptr == '\t'))
      sptr ++;

    if(*sptr == '=') {
      sptr ++;

      // Skip bad whitespace again, then parse the optional value.
      while((*sptr == ' ') || (*sptr == '\t'))
        sptr ++;

      tlen = this->m_value.parse(sptr);
      sptr += tlen;

      while((*sptr == ' ') || (*sptr == '\t'))
        sptr ++;
    }

    // The attribute shall be terminated by a separator.
    if((sptr != this->m_hstr.c_str() + this->m_hstr.ssize()) && (*sptr != ';') && (*sptr != ',')) {
      this->m_hpos = error_hpos;
      return -1;
    }

    // Accept this attribute and return the terminating separator.
    this->m_hpos = (size_t) (sptr - this->m_hstr.c_str());
    return (uint8_t) *sptr;
  }

bool
HTTP_Header_Parser::
next_attribute()
  {
    if(this->m_hpos == SIZE_MAX)
      return this->do_next_attribute_from_separator() >= 0;

    // If `m_hpos` is not before the beginning, it shall have stopped on a
    // separator. This function shall not move past an element separator.
    if(this->m_hpos >= this->m_hstr.size())
      return false;

    if(this->m_hstr[this->m_hpos] == ',')
      return false;

    if(this->m_hstr[this->m_hpos] != ';') {
      this->m_hpos = error_hpos;
      return false;
    }

    // Move past this attribute separator.
    return this->do_next_attribute_from_separator() >= 0;
  }

bool
HTTP_Header_Parser::
next_element()
  {
    if(this->m_hpos == SIZE_MAX)
      return this->do_next_attribute_from_separator() >= 0;

    // Skip attributes in this element.
    while((this->m_hpos < this->m_hstr.size()) && (this->m_hstr[this->m_hpos] == ';'))
      this->do_next_attribute_from_separator();

    // If `m_hpos` is not before the beginning, it shall have stopped on a
    // separator.
    if(this->m_hpos >= this->m_hstr.size())
      return false;

    if(this->m_hstr[this->m_hpos] != ',') {
      this->m_hpos = error_hpos;
      return false;
    }

    // Move past this attribute separator.
    return this->do_next_attribute_from_separator() >= 0;
  }

}  // namespace poseidon
