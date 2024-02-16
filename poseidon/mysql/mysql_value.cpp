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
        operator()(nullptr_t)
          {
            this->pfmt->putn("NULL", 4);
          }

        void
        operator()(int64_t num)
          {
            ::rocket::ascii_numput nump;
            nump.put_DI(num);
            this->pfmt->putn(nump.data(), nump.size());
          }

        void
        operator()(double num)
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
        operator()(cow_stringR str)
          {
            this->pfmt->putc('\'');
            auto pos = str.begin();
            while(pos != str.end()) {
              // https://dev.mysql.com/doc/refman/8.0/en/string-literals.html#character-escape-sequences
              static constexpr char src_chars[] = { '\0','\'','\"','\b','\n','\r','\t', 26,'\\' };
              static constexpr char esc_chars[] = {  '0','\'','\"', 'b', 'n', 'r', 't','Z','\\' };
              char my_esc_char = 0;

              for(uint32_t k = 0;  k != size(src_chars);  ++k)
                if(*pos == src_chars[k]) {
                  my_esc_char = esc_chars[k];
                  break;
                }

              if(my_esc_char != 0) {
                // Escape it.
                char seq[2] = { '\\', my_esc_char };
                ++ pos;
                this->pfmt->putn(seq, 2);
              }
              else {
                // Write this sequence verbatim.
                auto from = pos;
                pos = ::std::find_first_of(pos + 1, str.end(), begin(src_chars), end(src_chars));
                this->pfmt->putn(&*from, static_cast<size_t>(pos - from));
              }
            }
            this->pfmt->putc('\'');
          }

        void
        operator()(const DateTime_with_MYSQL_TIME& mdt)
          {
            ::timespec ts;
            ::rocket::ascii_numput ns_nump;
            timespec_from_system_time(ts, mdt.datetime.as_system_time());
            ns_nump.put_DU(static_cast<uint32_t>(ts.tv_nsec), 9);

            // `'1994-11-06 08:49:37.123'`
            char temp[32];
            temp[0] = '\'';
            mdt.datetime.print_iso8601_partial(temp + 1);
            temp[20] = '.';
            ::memcpy(temp + 21, ns_nump.data(), 4);
            ::memcpy(temp + 24, "\'\0*", 4);

            this->pfmt->putn(temp, 25);
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
