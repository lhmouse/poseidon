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
parse_quoted_string_partial(const char* str, size_t len)
  {
    if((len <= 1) || (*str != '\"'))
      return 0;

    this->m_str.clear();
    this->m_index = index_string;

    // Get a quoted string. Zero shall be returned in case of any errors.
    size_t plain = 1;
    const char* bp = str + 1;

    while(!plain || (*bp != '\"')) {
      // A previous escape character exists if `plain` equals zero.
      if(!plain || (*bp != '\\'))
        this->m_str.push_back(*bp);
      else
        plain = SIZE_MAX;

      plain ++;
      bp ++;

      // Fail if there is no closing quote.
      if(bp == str + len)
        return 0;
    }

    // Return the number of characters that have been parsed.
    return static_cast<size_t>(bp + 1 - str);
  }

size_t
HTTP_Value::
parse_datetime_partial(const char* str, size_t len)
  {
    size_t ac = this->m_dt.parse(str, len);
    this->m_index = index_datetime;
    return ac;
  }

size_t
HTTP_Value::
parse_number_partial(const char* str, size_t len)
  {
    ::rocket::ascii_numget numg;
    size_t ac = numg.parse_D(str, len);

    // Reject partial matches.
    if((ac < len) && !do_is_ctl_or_sep(str[ac]))
      return 0;

    numg.cast_D(this->m_num, -HUGE_VAL, +HUGE_VAL);
    this->m_index = index_number;
    return ac;
  }

size_t
HTTP_Value::
parse_token_partial(const char* str, size_t len)
  {
    auto pos = ::std::find_if(str, str + len, do_is_ctl_or_sep);
    this->m_str.assign(str, pos);
    this->m_index = index_string;
    return static_cast<size_t>(pos - str);
  }

size_t
HTTP_Value::
parse_unquoted_partial(const char* str, size_t len)
  {
    auto pos = ::std::find_if(str, str + len, do_is_ctl_or_unquoted_sep);
    this->m_str.assign(str, pos);
    this->m_index = index_string;
    return static_cast<size_t>(pos - str);
  }

size_t
HTTP_Value::
parse(const char* str, size_t len)
  {
    if(len == 0)
      return 0;

    size_t acc_len = 0;

    // What does the string look like?
    switch(*str) {
      case '\"':
        acc_len = this->parse_quoted_string_partial(str, len);
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        acc_len = this->parse_number_partial(str, len);
        break;

      case 'S':  // Sun, Sat
      case 'M':  // Mon
      case 'T':  // Tue, Thu
      case 'W':  // Wed
      case 'F':  // Fri
        acc_len = this->parse_datetime_partial(str, len);
        break;
    }

    // If previous attempts have failed, try this generic one.
    if(acc_len == 0)
      acc_len = this->parse_unquoted_partial(str, len);

    return acc_len;
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
