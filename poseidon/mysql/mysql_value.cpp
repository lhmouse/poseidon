// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "mysql_value.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

constexpr
bool
do_needs_escape(char ch) noexcept
  {
    // https://dev.mysql.com/doc/c-api/8.0/en/mysql-real-escape-string-quote.html
    return (ch == '\\') || (ch == '\'') || (ch == '\"') || (ch == 0)
           || (ch == '\n') || (ch == '\r') || (ch == 26) || (ch == '`');
  }

}  // namespace

MySQL_Value::
~MySQL_Value()
  {
  }

tinyfmt&
MySQL_Value::
print(tinyfmt& fmt) const
  {
    struct variant_printer
      {
        tinyfmt* pfmt;

        void
        operator()(nullptr_t) const
          {
            this->pfmt->putn("NULL", 4);
          }

        void
        operator()(int64_t num) const
          {
            ::rocket::ascii_numput nump;
            nump.put_DI(num);
            this->pfmt->putn(nump.data(), nump.size());
          }

        void
        operator()(double num) const
          {
            if(::std::isnan(num)) {
              // MySQL does not allow NaN values.
              this->pfmt->putn("NULL", 4);
              return;
            }

            double rval = ::std::clamp(num, -DBL_MAX, +DBL_MAX);
            ::rocket::ascii_numput nump;
            nump.put_DD(rval);
            this->pfmt->putn(nump.data(), nump.size());
          }

        void
        operator()(cow_stringR str) const
          {
            // First, write the prefix that needs no quoting.
            auto pos = ::std::find_if(str.begin(), str.end(), do_needs_escape);
            this->pfmt->putc('\'');
            this->pfmt->putn(str.data(), (size_t) (pos - str.begin()));

            while(pos != str.end()) {
              if((*pos == '\\') || (*pos == '\'') || (*pos == '\"') || (*pos == '`')) {
                // Escape it.
                char seq[2] = { '\\', *pos };
                this->pfmt->putn(seq, 2);
                ++ pos;
              }
              else if(*pos == 0) {
                // Translate it.
                this->pfmt->putn("\0", 2);
                ++ pos;
              }
              else if(*pos == '\n') {
                // Translate it.
                this->pfmt->putn("\\n", 2);
                ++ pos;
              }
              else if(*pos == '\r') {
                // Translate it.
                this->pfmt->putn("\\r", 2);
                ++ pos;
              }
              else if(*pos == 26) {
                // Translate it.
                this->pfmt->putn("\\Z", 2);
                ++ pos;
              }
              else {
                // Write this sequence verbatim.
                auto from = pos;
                pos = ::std::find_if(pos + 1, str.end(), do_needs_escape);
                this->pfmt->putn(&*from, (size_t) (pos - from));
              }
            }

            this->pfmt->putc('\'');
          }

        void
        operator()(const DateTime& dt) const
          {
            char sbuf[32];
            sbuf[0] = '\'';
            dt.print_iso8601_partial(sbuf + 1);
            sbuf[20] = '\'';
            this->pfmt->putn(sbuf, 21);
          }
      };

    variant_printer p = { &fmt };
    this->m_stor.visit(p);
    return fmt;
  }

cow_string
MySQL_Value::
print_to_string() const
  {
    tinyfmt_str fmt;
    this->print(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon