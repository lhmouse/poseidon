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

size_t
HTTP_Value::
parse_quoted_string_partial(chars_view str)
  {
    auto pstr = this->m_stor.mut_ptr<cow_string>();
    if(pstr)
      pstr->clear();
    else
      pstr = &(this->m_stor.emplace<cow_string>());

    // Get a quoted string. Zero shall be returned in case of any errors.
    if((str.n < 2) || (str[0] != '"'))
      return 0;

    const char* curp = str.p + 1;
    intptr_t plain = 1;

    while((*curp != '"') || (plain == 0)) {
      // A previous escape character exists if `plain` equals zero.
      if((*curp != '\\') || (plain == 0))
        pstr->push_back(*curp);
      else
        plain = -1;

      curp ++;
      plain ++;

      // Fail if there is no closing quote.
      if(curp == str.p + str.n)
        return 0;
    }

    // Skip the closing quote.
    return (size_t) (curp + 1 - str.p);
  }

size_t
HTTP_Value::
parse_number_partial(chars_view str)
  {
    auto pnum = this->m_stor.mut_ptr<double>();
    if(pnum)
      *pnum = 0;
    else
      pnum = &(this->m_stor.emplace<double>());

    // Parse a floating-point number. Do not mistake a token prefix.
    ::rocket::ascii_numget numg;
    size_t aclen = numg.parse_D(str.p, str.n);
    if((aclen < str.n) && !do_is_ctl_or_sep(str[aclen]))
      return 0;

    numg.cast_D(*pnum, -HUGE_VAL, +HUGE_VAL);
    return aclen;
  }

size_t
HTTP_Value::
parse_datetime_partial(chars_view str)
  {
    auto pdt = this->m_stor.mut_ptr<DateTime>();
    if(pdt)
      pdt->set_system_time(system_time());
    else
      pdt = &(this->m_stor.emplace<DateTime>());

    size_t aclen = pdt->parse(str);
    return aclen;
  }

size_t
HTTP_Value::
parse_token_partial(chars_view str)
  {
    auto pstr = this->m_stor.mut_ptr<cow_string>();
    if(pstr)
      pstr->clear();
    else
      pstr = &(this->m_stor.emplace<cow_string>());

    auto eptr = ::std::find_if(str.p, str.p + str.n, do_is_ctl_or_sep);
    pstr->assign(str.p, eptr);
    return (size_t) (eptr - str.p);
  }

size_t
HTTP_Value::
parse_unquoted_partial(chars_view str)
  {
    auto pstr = this->m_stor.mut_ptr<cow_string>();
    if(pstr)
      pstr->clear();
    else
      pstr = &(this->m_stor.emplace<cow_string>());

    auto eptr = ::std::find_if(str.p, str.p + str.n, do_is_ctl_or_unquoted_sep);
    pstr->assign(str.p, eptr);
    return (size_t) (eptr - str.p);
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
print_to(tinyfmt& fmt) const
  {
    struct variant_printer
      {
        tinyfmt* pfmt;

        void
        operator()(nullptr_t)
          {
          }

        void
        operator()(const cow_string& str)
          {
            auto pos = ::std::find_if(str.begin(), str.end(), do_is_ctl_or_sep);
            if(pos == str.end()) {
              // Write the string unquoted.
              this->pfmt->putn(str.data(), str.size());
              return;
            }

            this->pfmt->putc('"');
            this->pfmt->putn(str.data(), (size_t) (pos - str.begin()));
            while(pos != str.end()) {
              if((*pos == '\\') || (*pos == '"')) {
                // Escape it.
                char seq[2] = { '\\', *pos };
                ++ pos;
                this->pfmt->putn(seq, 2);
              }
              else if(do_is_ctl_or_ws(*pos)) {
                // Replace this sequence of control and space characters with a
                // single space.
                this->pfmt->putc(' ');
                pos = ::std::find_if_not(pos + 1, str.end(), do_is_ctl_or_ws);
              }
              else {
                // Write this sequence verbatim.
                auto from = pos;
                pos = ::std::find_if(pos + 1, str.end(), do_is_ctl_or_sep);
                this->pfmt->putn(&*from, (size_t) (pos - from));
              }
            }
            this->pfmt->putc('"');
          }

        void
        operator()(double num)
          {
            ::rocket::ascii_numput nump;
            nump.put_DD(num);
            this->pfmt->putn(nump.data(), nump.size());
          }

        void
        operator()(const DateTime& dt)
          {
            char sbuf[32];
            dt.print_rfc1123_partial(sbuf);
            this->pfmt->putn(sbuf, 29);
          }
      };

    variant_printer p = { &fmt };
    this->m_stor.visit(p);
    return fmt;
  }

cow_string
HTTP_Value::
print_to_string() const
  {
    tinyfmt_str fmt;
    this->print_to(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
