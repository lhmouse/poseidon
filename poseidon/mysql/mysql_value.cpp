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
print_to(tinyfmt& fmt) const
  {
    switch(this->type())
      {
      case mysql_value_null:
        fmt << "NULL";
        break;

      case mysql_value_integer:
        fmt << this->as_integer();
        break;

      case mysql_value_double:
        {
          if(::std::isnan(this->as_double()))
            fmt << "NULL";
          else
            fmt << ::std::clamp(this->as_double(), -DBL_MAX, DBL_MAX);
        }
        break;

      case mysql_value_blob:
        {
          const auto& str = this->as_blob();
          fmt.putc('\'');
          size_t pos = str.find_of("\'\\");
          fmt.putn(str.data(), ::std::min(pos, str.size()));
          while(pos != str.npos) {
            size_t base = pos;
            pos = str.find_of(pos + 1, "\'\\");
            fmt.putc('\\');
            fmt.putn(str.data() + base, ::std::min(pos, str.size()) - base);
          }
          fmt.putc('\'');
        }
        break;

      case mysql_value_datetime:
        {
          // `'1994-11-06 08:49:37.123'`
          char temp[32];
          temp[0] = '\'';
          this->as_datetime().print_iso8601_ns_partial(temp + 1);
          temp[11] = ' ';
          temp[24] = '\'';
          fmt.putn(temp, 25);
        }
        break;

      default:
        ASTERIA_TERMINATE(("Corrupted value type `$1`"), this->m_stor.index());
      }

    return fmt;
  }

cow_string
MySQL_Value::
print_to_string() const
  {
    tinyfmt_str fmt;
    this->print_to(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
