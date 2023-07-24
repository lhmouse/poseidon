// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_value.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

constexpr
bool
do_is_ctl_or_sep(char ch) noexcept
  {
    // https://www.rfc-editor.org/rfc/rfc2616#section-2.2
    return ((ch >= 0x00) && (ch <= 0x20)) || (ch == 0x7F)
           || (ch == '\\') || (ch == '\"')
           || (ch == '(') || (ch == ')') || (ch == '<') || (ch == '>')
           || (ch == '@') || (ch == ',') || (ch == ';') || (ch == ':')
           || (ch == '/') || (ch == '[') || (ch == ']') || (ch == '?')
           || (ch == '=') || (ch == '{') || (ch == '}');
  }

constexpr
bool
do_is_ctl_or_unquoted_sep(char ch) noexcept
  {
    return ((ch >= 0x00) && (ch <= 0x20)) || (ch == 0x7F)
           || (ch == ',') || (ch == ';');
  }

constexpr
bool
do_is_ctl_or_ws(char ch) noexcept
  {
    return ((ch >= 0x00) && (ch <= 0x20)) || (ch == 0x7F);
  }

}  // namespace

HTTP_Value::
~HTTP_Value()
  {
  }

size_t
HTTP_Value::
parse_quoted_string_partial(chars_view str)
  {
    if((str.n == 0) || (*str != '\"'))
      return 0;

    this->m_str.clear();
    this->m_index = index_string;

    // Get a quoted string. Zero shall be returned in case of any errors.
    const char* curp = str.p + 1;
    intptr_t plain = 1;

    while((*curp != '\"') || (plain == 0)) {
      // A previous escape character exists if `plain` equals zero.
      if((*curp != '\\') || (plain == 0))
        this->m_str.push_back(*curp);
      else
        plain = -1;

      curp ++;
      plain ++;

      // Fail if there is no closing quote.
      if(curp == str.p + str.n)
        return 0;
    }

    // Return the number of characters that have been parsed.
    return (size_t) (curp + 1 - str.p);
  }

size_t
HTTP_Value::
parse_datetime_partial(chars_view str)
  {
    size_t aclen = this->m_dt.parse(str);
    this->m_index = index_datetime;
    return aclen;
  }

size_t
HTTP_Value::
parse_number_partial(chars_view str)
  {
    ::rocket::ascii_numget numg;
    size_t aclen = numg.parse_D(str.p, str.n);

    // Do not mistake a token prefix.
    if((aclen != str.n) && !do_is_ctl_or_sep(str.p[aclen]))
      return 0;

    numg.cast_D(this->m_num, -HUGE_VAL, +HUGE_VAL);
    this->m_index = index_number;
    return aclen;
  }

size_t
HTTP_Value::
parse_token_partial(chars_view str)
  {
    auto eptr = ::std::find_if(str.p, str.p + str.n, do_is_ctl_or_sep);
    size_t aclen = (size_t) (eptr - str.p);

    this->m_str.assign(str.p, aclen);
    this->m_index = index_string;
    return aclen;
  }

size_t
HTTP_Value::
parse_unquoted_partial(chars_view str)
  {
    auto eptr = ::std::find_if(str.p, str.p + str.n, do_is_ctl_or_unquoted_sep);
    size_t aclen = (size_t) (eptr - str.p);

    this->m_str.assign(str.p, aclen);
    this->m_index = index_string;
    return aclen;
  }

size_t
HTTP_Value::
parse(chars_view str)
  {
    if(str.n == 0)
      return 0;

    // What does the string look like?
    if(*str == '\"')
      if(size_t aclen = this->parse_quoted_string_partial(str))
        return aclen;

    if((*str >= '0') && (*str <= '9'))
      if(size_t aclen = this->parse_number_partial(str))
        return aclen;

    if((*str >= 'F') && (*str <= 'W'))
      if(size_t aclen = this->parse_datetime_partial(str))
        return aclen;

    // This is the fallback for everything.
    if(size_t aclen = this->parse_unquoted_partial(str))
      return aclen;

    return 0;
  }

tinyfmt&
HTTP_Value::
print(tinyfmt& fmt) const
  {
    switch(this->m_index) {
      case index_null:
        // Don't write anything.
        return fmt;

      case index_string: {
        auto pos = ::std::find_if(this->m_str.begin(), this->m_str.end(), do_is_ctl_or_sep);
        if(pos != this->m_str.end()) {
          // Quote the string.
          fmt.putc('\"');
          fmt.putn(this->m_str.data(), (size_t) (pos - this->m_str.begin()));
          do {
            if((*pos == '\\') || (*pos == '\"')) {
              // Escape it.
              char seq[2] = { '\\', *pos };
              fmt.putn(seq, 2);
              ++ pos;
            }
            else if(do_is_ctl_or_ws(*pos)) {
              // Replace this sequence of control and space characters
              // with a single space.
              fmt.putc(' ');
              pos = ::std::find_if_not(pos + 1, this->m_str.end(), do_is_ctl_or_ws);
            }
            else {
              // Write this sequence verbatim.
              auto from = pos;
              pos = ::std::find_if(pos + 1, this->m_str.end(), do_is_ctl_or_sep);
              fmt.putn(&*from, (size_t) (pos - from));
            }
          }
          while(pos != this->m_str.end());
          fmt.putc('\"');
        }
        else {
          // Write the string verbatim.
          fmt << this->m_str;
        }
        return fmt;
      }

      case index_number:
        // Write the number verbatim.
        fmt << this->m_num;
        return fmt;

      case index_datetime:
        // Write the date/time verbatim.
        fmt << this->m_dt;
        return fmt;

      default:
        ASTERIA_TERMINATE((
            "Invalid HTTP value type `$1`"),
            this->m_index);
    }
  }

cow_string
HTTP_Value::
print_to_string() const
  {
    tinyfmt_str fmt;
    this->print(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
