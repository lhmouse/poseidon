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
    return ((ch >= 0x00) && (ch <= 0x20)) || (ch == '(') || (ch == ')')
           || (ch == '<') || (ch == '>') || (ch == '@') || (ch == ',')
           || (ch == ';') || (ch == ':') || (ch == '\\') || (ch == '\"')
           || (ch == '/') || (ch == '[') || (ch == ']') || (ch == '?')
           || (ch == '=') || (ch == '{') || (ch == '}');
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

    // Get a quoted string. Zero shall be returned in case of any errors.
    cow_string unescaped;
    size_t plain = 1;
    const char* bp = str + 1;

    while(!plain || (*bp != '\"')) {
      // A previous escape character exists if `plain` equals zero.
      if(!plain || (*bp != '\\'))
        unescaped.push_back(*bp);
      else
        plain = SIZE_MAX;

      plain ++;
      bp ++;

      // Fail if there is no closing quote.
      if(bp == str + len)
        return 0;
    }

    this->m_stor = ::std::move(unescaped);
    return (size_t) (bp + 1 - str);
  }

size_t
HTTP_Value::
parse_datetime_partial(const char* str, size_t len)
  {
    HTTP_DateTime tm;
    size_t ac = tm.parse(str, len);
    if(ac == 0)
      return 0;

    this->m_stor = tm;
    return ac;
  }

size_t
HTTP_Value::
parse_number_partial(const char* str, size_t len)
  {
    ::rocket::ascii_numget numg;
    size_t ac = numg.parse_D(str, len);
    if(ac == 0)
      return 0;

    // Make sure it's not part of a token.
    if((ac < len) && !do_is_ctl_or_sep(str[ac]))
      return 0;

    double num;
    numg.cast_D(num, -HUGE_VAL, +HUGE_VAL);
    this->m_stor = num;
    return ac;
  }

size_t
HTTP_Value::
parse_token_partial(const char* str, size_t len)
  {
    auto pos = ::std::find_if(str, str + len, do_is_ctl_or_sep);
    if(pos == str)
      return 0;

    this->m_stor = cow_string(str, pos);
    return (size_t) (pos - str);
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
      acc_len = this->parse_token_partial(str, len);

    return acc_len;
  }

tinyfmt&
HTTP_Value::
print(tinyfmt& fmt) const
  {
    if(this->is_null())
      return fmt;

    if(this->is_string()) {
      // Check whether the string has to be quoted.
      const auto& str = this->m_stor.as<cow_string>();
      auto pos = ::std::find_if(str.begin(), str.end(), do_is_ctl_or_sep);
      if(pos != str.end()) {
        fmt.putc('\"');

        fmt.putn(str.data(), (size_t) (pos - str.begin()));
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
            pos = ::std::find_if_not(pos + 1, str.end(), do_is_ctl_or_ws);
          }
          else {
            // Write this sequence verbatim.
            auto from = pos;
            pos = ::std::find_if(pos + 1, str.end(), do_is_ctl_or_sep);
            fmt.putn(&*from, (size_t) (pos - from));
          }
        }
        while(pos != str.end());

        fmt.putc('\"');
        return fmt;
      }
    }

    // Write the value verbatim.
    this->m_stor.visit([&](const auto& val) { fmt << val;  });
    return fmt;
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
