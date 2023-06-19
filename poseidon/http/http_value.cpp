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
    return ((ch >= 0x00) && (ch <= 0x20)) || (ch == '(') || (ch == ')') || (ch == '<')
           || (ch == '>') || (ch == '@') || (ch == ',') || (ch == ';') || (ch == ':')
           || (ch == '\\') || (ch == '\"') || (ch == '/') || (ch == '[') || (ch == ']')
           || (ch == '?') || (ch == '=') || (ch == '{') || (ch == '}');
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
      cow_string unesc;
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
      if((r != 0) && do_is_ctl_or_sep(str[r])) {
        double num;
        numg.cast_D(num, -HUGE_VAL, +HUGE_VAL);
        this->m_stor = num;
        return r;
      }
    }

    // Try a token.
    auto etok = ::std::find_if(str, str + len, do_is_ctl_or_sep);
    if(etok != str) {
      this->m_stor = cow_string(str, etok);
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
parse(cow_stringR str)
  {
    return this->parse(str.data(), str.size());
  }

tinyfmt&
HTTP_Value::
print(tinyfmt& fmt) const
  {
    if(auto qstr = this->m_stor.ptr<cow_string>()) {
      // Check whether the string shall be quoted.
      auto pos = ::std::find_if(qstr->begin(), qstr->end(), do_is_ctl_or_sep);
      if(pos == qstr->end()) {
        // If the string is a valid token, write it verbatim.
        fmt.putn(qstr->data(), qstr->size());
      }
      else {
        // ... no, so quote it.
        fmt.putc('\"');
        fmt.putn(qstr->data(), (size_t) (pos - qstr->begin()));
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
            pos = ::std::find_if_not(pos + 1, qstr->end(), do_is_ctl_or_ws);
          }
          else {
            // Write this sequence verbatim.
            auto from = pos;
            pos = ::std::find_if(pos + 1, qstr->end(), do_is_ctl_or_sep);
            fmt.putn(&*from, (size_t) (pos - from));
          }
        }
        while(pos != qstr->end());
        fmt.putc('\"');
      }
      return fmt;
    }

    if(auto qnum = this->m_stor.ptr<double>())
      return fmt << *qnum;

    if(auto qtm = this->m_stor.ptr<HTTP_DateTime>())
      return fmt << *qtm;

    // The value is null, so don't print anything.
    ROCKET_ASSERT(this->m_stor.index() == 0);
    return fmt;
  }

cow_string
HTTP_Value::
print_to_string() const
  {
    ::rocket::tinyfmt_str fmt;
    this->print(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
