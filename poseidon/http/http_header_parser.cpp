// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_header_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Header_Parser::
HTTP_Header_Parser() noexcept
  {
  }

HTTP_Header_Parser::
~HTTP_Header_Parser()
  {
  }

void
HTTP_Header_Parser::
clear() noexcept
  {
    this->m_hstr.clear();
    this->m_hpos = 0;

    this->m_name.clear();
    this->m_value.clear();
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
    const char* const esptr = this->m_hstr.c_str() + this->m_hstr.size();

    // Skip leading whitespace. This function shall not move across element
    // boundaries.
    while((*sptr == ' ') || (*sptr == '\t'))
      sptr ++;

    if((sptr == esptr) || (*sptr == ';') || (*sptr == ','))
      return -1;

    // Parse the name of an attribute, and initialize its value to null.
    size_t tlen = this->m_value.parse_token_partial(sptr);
    if(tlen == 0) {
      POSEIDON_LOG_DEBUG(("Invalid attribute name at `$1`"), chars_view(sptr, ::strnlen(sptr, 40)));
      this->m_hpos = error_hpos;
      return -1;
    }

    sptr += tlen;
    this->m_name.swap(this->m_value.mut_string());
    this->m_value = nullptr;

    // If an equals sign is encountered, then there will be a value, so
    // parse it.
    while((*sptr == ' ') || (*sptr == '\t'))
      sptr ++;

    if(*sptr == '=') {
      sptr ++;

      while((*sptr == ' ') || (*sptr == '\t'))
        sptr ++;

      tlen = this->m_value.parse(sptr);
      sptr += tlen;

      // Ensure the value is not null in this case, so it's distinguishable
      // from not having a value.
      if(tlen == 0)
        this->m_value.set_string(&"");
    }

    // Skip trailing whitespace.
    while((*sptr == ' ') || (*sptr == '\t'))
      sptr ++;

    // The attribute shall have been terminated by a separator.
    if((sptr != esptr) && (*sptr != ';') && (*sptr != ',')) {
      POSEIDON_LOG_DEBUG(("Invalid character encountered at `$1`"), chars_view(sptr, ::strnlen(sptr, 40)));
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
    // The first call shall retrieve the first attribute.
    if(this->m_hpos == SIZE_MAX)
      return this->do_next_attribute_from_separator() >= 0;

    if(this->m_hpos >= this->m_hstr.size()) {
      // If `m_hpos` equals `m_hstr.size()` then the end of the input
      // string has been reached; otherwise it indicates an error, so
      // don't touch it.
      return false;
    }
    else
      switch(this->m_hstr.at(this->m_hpos)) {
      case ',':
        // Stop at this element separator.
        return false;

      case ';':
        // Move past this attribute separator.
        return this->do_next_attribute_from_separator() >= 0;

      default:
        this->m_hpos = error_hpos;
        return false;
      }
  }

bool
HTTP_Header_Parser::
next_element()
  {
    // The first call shall retrieve the first attribute.
    if(this->m_hpos == SIZE_MAX)
      return this->do_next_attribute_from_separator() >= 0;

    for(;;)
      if(this->m_hpos >= this->m_hstr.size()) {
        // If `m_hpos` equals `m_hstr.size()` then the end of the input
        // string has been reached; otherwise it indicates an error, so
        // don't touch it.
        return false;
      }
      else
        switch(this->m_hstr.at(this->m_hpos)) {
        case ',':
          // Move past this element separator.
          return this->do_next_attribute_from_separator() >= 0;

        case ';':
          // Move past this attribute separator.
          this->do_next_attribute_from_separator();
          continue;

        default:
          this->m_hpos = error_hpos;
          return false;
        }
  }

}  // namespace poseidon
