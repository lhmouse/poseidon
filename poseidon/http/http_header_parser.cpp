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

int
HTTP_Header_Parser::
do_next_attribute_from_separator()
  {
    // Skip the current separator.
    size_t& hpos = this->m_hpos;
    hpos ++;
    ROCKET_ASSERT(hpos <= this->m_hstr.size());

    // Skip whitespace and attribute separators. This function shall not move
    // across element boundaries.
    while((this->m_hstr[hpos] == ' ') || (this->m_hstr[hpos] == '\t') || (this->m_hstr[hpos] == ';'))
      hpos ++;

    if((this->m_hstr[hpos] == ',') || (hpos >= this->m_hstr.size()))
      return -1;

    // Parse the name of an attribute, and initialize its value to null.
    size_t tlen = this->m_value.parse_token_partial(this->m_hstr.data() + hpos, this->m_hstr.size() - hpos);
    if(tlen == 0) {
      this->m_hpos = hpos_error;
      return -1;
    }

    hpos += tlen;
    this->m_name = ::std::move(this->m_value.mut_string());
    this->m_value = nullptr;

    // Skip bad whitespace.
    while((this->m_hstr[hpos] == ' ') || (this->m_hstr[hpos] == '\t'))
      hpos ++;

    if(this->m_hstr[hpos] == '=') {
      hpos ++;

      // Parse the optional value.
      while((this->m_hstr[hpos] == ' ') || (this->m_hstr[hpos] == '\t'))
        hpos ++;

      tlen = this->m_value.parse(this->m_hstr.data() + hpos, this->m_hstr.size() - hpos);
      hpos += tlen;

      while((this->m_hstr[hpos] == ' ') || (this->m_hstr[hpos] == '\t'))
        hpos ++;
    }

    // The attribute shall be terminated by a separator.
    if((this->m_hstr[hpos] != ';') && (this->m_hstr[hpos] != ',') && (hpos != this->m_hstr.size())) {
      this->m_hpos = hpos_error;
      return -1;
    }

    // Accept this attribute and return the current separator.
    return (uint8_t) this->m_hstr[hpos];
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
      this->m_hpos = hpos_error;
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
      this->m_hpos = hpos_error;
      return false;
    }

    // Move past this attribute separator.
    return this->do_next_attribute_from_separator() >= 0;
  }

}  // namespace poseidon
