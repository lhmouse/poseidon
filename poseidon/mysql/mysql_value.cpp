// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_value.hpp"
#include "../utils.hpp"
namespace poseidon {

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
            this->pfmt->putc('\'');

            size_t pos = 0;
            while(pos != str.size()) {
              // This includes the null terminator.
              // https://dev.mysql.com/doc/c-api/8.0/en/mysql-real-escape-string-quote.html
              static constexpr char unsafe_chars[] = "\\\'\"\n\r\x1A`";
              size_t brk = str.find_of(pos, unsafe_chars, sizeof(unsafe_chars));
              if(brk == cow_string::npos) {
                this->pfmt->putn(str.data() + pos, str.size() - pos);
                break;
              }

              this->pfmt->putn(str.data() + pos, brk - pos);
              pos = brk + 1;
              char eseq[2] = { '\\', str[brk] };

              // Escape this single character.
              switch(eseq[1])
                {
                case '\n':
                  this->pfmt->putn("\\n", 2);
                  break;

                case '\r':
                  this->pfmt->putn("\\r", 2);
                  break;

                case 0x1A:
                  this->pfmt->putn("\\Z", 2);
                  break;

                case 0:
                  this->pfmt->putn("\\0", 2);
                  break;

                default:
                  this->pfmt->putn(eseq, 2);
                  break;
                }
            }

            this->pfmt->putc('\'');
          }

        void
        operator()(const ::MYSQL_TIME& myt) const
          {
            this->pfmt->putc('\'');

            ::rocket::ascii_numput nump;
            nump.put_DU(myt.year, 4);
            this->pfmt->putn(nump.data(), nump.size());

            this->pfmt->putc('-');
            nump.put_DU(myt.month, 2);
            this->pfmt->putn(nump.data(), nump.size());

            this->pfmt->putc('-');
            nump.put_DU(myt.day, 2);
            this->pfmt->putn(nump.data(), nump.size());

            this->pfmt->putc(' ');
            nump.put_DU(myt.hour, 2);
            this->pfmt->putn(nump.data(), nump.size());

            this->pfmt->putc(':');
            nump.put_DU(myt.minute, 2);
            this->pfmt->putn(nump.data(), nump.size());

            this->pfmt->putc(':');
            nump.put_DU(myt.second, 2);
            this->pfmt->putn(nump.data(), nump.size());

            if(myt.second_part != 0) {
              // This part is optional and is usually omitted.
              this->pfmt->putc('.');
              nump.put_DU(myt.second_part, 3);
              this->pfmt->putn(nump.data(), nump.size());
            }

            this->pfmt->putc('\'');
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
