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
    return ((ch >= 0x00) && (ch <= 0x1F))
           || (ch == '(') || (ch == ')') || (ch == '<') || (ch == '>') || (ch == '@')
           || (ch == ',') || (ch == ';') || (ch == ':') || (ch == '\\') || (ch == '\"')
           || (ch == '/') || (ch == '[') || (ch == ']') || (ch == '?') || (ch == '=')
           || (ch == '{') || (ch == '}') || (ch == ' ') || (ch == '\t');
  }

constexpr
bool
do_is_ctl_or_ws(char ch) noexcept
  {
    return (ch >= 0x00) && (ch <= 0x20);
  }

}  // namespace

size_t
HTTP_Value::
parse(const char* str, size_t len)
  {
    this->m_stor = nullptr;

    if(len == 0)
      return 0;

    if((*str == 'S') || (*str == 'M') || (*str == 'T') || (*str == 'W') || (*str == 'F')) {
      // This might start a weekday. Try HTTP date/time.
      HTTP_DateTime tm;
      size_t r = tm.parse(str, len);
      if(r) {
        this->m_stor = tm;
        return r;
      }
    }

    if(*str == '\"') {
      // Get a quoted string.
      string unesc;
      bool escaped = false;
      size_t sp = 0;
      while(++ sp != len) {
        // Escaped characters, including control characters, are accepted
        // unconditionally for maximum compatibility.
        if(!escaped && (str[sp] == '\\')) {
          escaped = true;
          continue;
        }
        else if(!escaped && (str[sp] == '\"')) {
          this->m_stor = ::std::move(unesc);
          return sp + 1;
        }
        unesc.push_back(str[sp]);
        escaped = false;
      }
    }

    if((*str >= '0') && (*str <= '9')) {
      // This might start a number. Try parsing a `double`, but also make
      // sure it is not part of a token.
      ::rocket::ascii_numget numg;
      size_t r = numg.parse_D(str, len);
      if(r && ((r == len) || do_is_ctl_or_sep(str[r]))) {
        double num;
        numg.cast_D(num, -HUGE_VAL, +HUGE_VAL);
        this->m_stor = num;
        return r;
      }
    }

    // Try a token.
    auto etok = ::std::find_if(str, str + len, do_is_ctl_or_sep);
    if(etok != str) {
      this->m_stor = string(str, etok);
      return (size_t) (etok - str);
    }

    // Nothing has been accepted. Fail.
    return 0;
  }

size_t
HTTP_Value::
parse(const char* str)
  {
    return this->parse(str, ::strlen(str));
  }

size_t
HTTP_Value::
parse(stringR str)
  {
    return this->parse(str.data(), str.size());
  }

tinyfmt&
HTTP_Value::
print(tinyfmt& fmt) const
  {
    switch(this->index()) {
      case index_null:
        return fmt;

      case index_string: {
        // Check whether the string is to be quoted.
        const auto& str = this->as_string();
        auto pos = ::std::find_if(str.begin(), str.end(), do_is_ctl_or_sep);

        // If the string is a valid token, write it verbatim.
        if(pos == str.end())
          return fmt << str;

        // ... no, so quote it.
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

      case index_number:
        return fmt << this->as_number();

      case index_datetime:
        return fmt << this->as_datetime();

      default:
        ROCKET_ASSERT(false);
    }
  }

string
HTTP_Value::
print_to_string() const
  {
    ::rocket::tinyfmt_str fmt;
    this->print(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
