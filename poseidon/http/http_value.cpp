// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
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
           || (ch == '\\') || (ch == '"')
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

void
HTTP_Value::
do_update_variants()
  {
    if(this->m_vm & vm_int_valid) {
      // Initialize others from integer.
      ::rocket::ascii_numput nump;
      nump.put_DI(this->m_int);
      this->m_str.assign(nump.c_str(), nump.length());
      this->m_vm = vm_str_valid | vm_int_valid;

      this->m_dbl = static_cast<double>(this->m_int);
      this->m_vm |= vm_dbl_valid;

      this->m_dt = system_time();
    }
    else if(this->m_vm & vm_dbl_valid) {
      // Initialize others from double.
      ::rocket::ascii_numput nump;
      nump.put_DD(this->m_dbl);
      this->m_str.assign(nump.c_str(), nump.length());
      this->m_vm = vm_str_valid | vm_dbl_valid;

      ::feclearexcept(FE_ALL_EXCEPT);
      this->m_int = ::llrint(this->m_dbl);
      if(::fetestexcept(FE_ALL_EXCEPT) == 0)
        this->m_vm |= vm_int_valid;

      this->m_dt = system_time();
    }
    else if(this->m_vm & vm_dt_valid) {
      // Initialize others from date-time.
      char tempstr[30];
      this->m_dt.print_rfc1123_partial(tempstr);
      this->m_str.assign(tempstr, 29);
      this->m_vm = vm_str_valid | vm_dt_valid;

      this->m_int = 0;
      this->m_dbl = 0;
    }
    else if(this->m_vm & vm_str_valid) {
      // Initialize others from string.
      this->m_vm = vm_str_valid;
      size_t b = this->m_str.find_not_of(" \t");
      if(b != cow_string::npos) {
        chars_view sv(this->m_str.c_str() + b, this->m_str.rfind_not_of(" \t") - b + 1);
        ::rocket::ascii_numget numg;

        // Try parsing.
        if(numg.parse_I(sv.p, sv.n) == sv.n) {
          numg.cast_I(this->m_int, INT64_MIN, INT64_MAX);
          this->m_vm |= vm_int_valid;
        }

        if(numg.parse_D(sv.p, sv.n) == sv.n) {
          numg.cast_D(this->m_dbl, -HUGE_VAL, HUGE_VAL);
          this->m_vm |= vm_dbl_valid;
        }

        if(this->m_dt.parse(sv) == sv.n)
          this->m_vm |= vm_dt_valid;
      }
    }
  }

size_t
HTTP_Value::
parse_quoted_string_partial(chars_view str)
  {
    this->clear();

    // Get a quoted string. Zero shall be returned in case of any errors.
    if((str.n < 2) || (str[0] != '"'))
      return 0;

    const char* curp = str.p + 1;
    intptr_t plain = 1;

    while((*curp != '"') || (plain == 0)) {
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

    this->m_vm = vm_str_valid;
    this->do_update_variants();
    return static_cast<size_t>(curp + 1 - str.p);
  }

size_t
HTTP_Value::
parse_datetime_partial(chars_view str)
  {
    this->clear();

    size_t aclen = this->m_dt.parse(str);
    if(aclen == 0)
      return 0;

    this->m_str.assign(str.p, str.n);
    this->m_int = 0;
    this->m_dbl = 0;
    this->m_vm = vm_dt_valid;
    return aclen;
  }

size_t
HTTP_Value::
parse_token_partial(chars_view str)
  {
    this->clear();

    auto eptr = ::std::find_if(str.p, str.p + str.n, do_is_ctl_or_sep);
    this->m_str.assign(str.p, eptr);
    if(this->m_str.empty())
      return 0;

    this->m_vm = vm_str_valid;
    this->do_update_variants();
    return static_cast<size_t>(eptr - str.p);
  }

size_t
HTTP_Value::
parse_unquoted_partial(chars_view str)
  {
    this->clear();

    auto eptr = ::std::find_if(str.p, str.p + str.n, do_is_ctl_or_unquoted_sep);
    this->m_str.assign(str.p, eptr);
    if(this->m_str.empty())
      return 0;

    this->m_vm = vm_str_valid;
    this->do_update_variants();
    return static_cast<size_t>(eptr - str.p);
  }

size_t
HTTP_Value::
parse(chars_view str)
  {
    if(str.n == 0)
      return 0;

    // What does the string look like?
    if(*str == '"')
      if(size_t aclen = this->parse_quoted_string_partial(str))
        return aclen;

    if(size_t aclen = this->parse_datetime_partial(str))
      return aclen;

    if(size_t aclen = this->parse_unquoted_partial(str))
      return aclen;

    return 0;
  }

tinyfmt&
HTTP_Value::
print_to(tinyfmt& fmt) const
  {
    if(this->m_vm & vm_int_valid)
      return fmt << this->m_int;
    else if(this->m_vm & vm_dbl_valid)
      return fmt << this->m_dbl;
    else if(this->m_vm & vm_dt_valid) {
      char tempstr[30];
      this->m_dt.print_rfc1123_partial(tempstr);
      return fmt << tempstr;
    }
    else {
      // Check whether the string has to be quoted.
      auto eptr = this->m_str.data() + this->m_str.size();
      auto pos = ::std::find_if(this->m_str.data(), eptr, do_is_ctl_or_sep);
      if(pos == eptr)
        fmt.putn(this->m_str.data(), this->m_str.size());
      else {
        fmt.putc('"');
        fmt.putn(this->m_str.data(), static_cast<size_t>(pos - this->m_str.data()));
        do {
          if((*pos == '\\') || (*pos == '"')) {
            // Escape it.
            char seq[2] = { '\\', *pos };
            ++ pos;
            fmt.putn(seq, 2);
          }
          else if(do_is_ctl_or_ws(*pos)) {
            // Replace this sequence of control and space characters with a
            // single space.
            fmt.putc(' ');
            pos = ::std::find_if_not(pos + 1, eptr, do_is_ctl_or_ws);
          }
          else {
            // Write this sequence verbatim.
            auto from = pos;
            pos = ::std::find_if(pos + 1, eptr, do_is_ctl_or_sep);
            fmt.putn(&*from, static_cast<size_t>(pos - from));
          }
        }
        while(pos != eptr);
        fmt.putc('"');
      }
      return fmt;
    }
  }

cow_string
HTTP_Value::
to_string() const
  {
    tinyfmt_str fmt;
    this->print_to(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
