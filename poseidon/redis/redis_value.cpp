// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_value.hpp"
#include "../utils.hpp"
namespace poseidon {

Redis_Value::
~Redis_Value()
  {
  }

tinyfmt&
Redis_Value::
print(tinyfmt& fmt) const
  {
    struct variant_printer
      {
        tinyfmt* pfmt;

        void
        operator()(nullptr_t)
          {
            this->pfmt->putn("(nil)", 4);
          }

        void
        operator()(int64_t num)
          {
            ::rocket::ascii_numput nump;
            nump.put_DI(num);
            this->pfmt->putn(nump.data(), nump.size());
          }

        void
        operator()(cow_stringR str)
          {
            this->pfmt->putc('\"');
            size_t offset = 0;
            while(offset < str.size()) {
              // Decode one UTF code point.
              char32_t cp;
              if(!::asteria::utf8_decode(cp, str, offset)) {
                cp = U'\xFFFD';
                offset ++;
              }

              switch(cp)
                {
                case '\b':
                  this->pfmt->putn("\\b", 2);
                  break;

                case '\f':
                  this->pfmt->putn("\\f", 2);
                  break;

                case '\n':
                  this->pfmt->putn("\\n", 2);
                  break;

                case '\r':
                  this->pfmt->putn("\\r", 2);
                  break;

                case '\t':
                  this->pfmt->putn("\\t", 2);
                  break;

                case 0x20 ... 0x7E:
                  {
                    if(cp == '\"')
                      this->pfmt->putn("\\\"", 2);
                    else if(cp == '\\')
                      this->pfmt->putn("\\\"", 2);
                    else
                      this->pfmt->putc(static_cast<char>(cp));
                  }
                  break;

                default:
                  {
                    char16_t ustr[2];
                    char16_t* ueptr = ustr;
                    ::asteria::utf16_encode(ueptr, cp);

                    char u16seq[16] = "\\uXXXX\\uXXXX";
                    ::rocket::ascii_numput nump;
                    nump.put_XU(ustr[0] * 0x10000UL + ustr[1], 8);
                    ::memcpy(u16seq + 2, nump.data(), 4);
                    ::memcpy(u16seq + 8, nump.data() + 4, 4);

                    this->pfmt->putn(u16seq, static_cast<size_t>(ueptr - ustr) * 6);
                  }
                  break;
                }
            }
            this->pfmt->putc('\"');
          }

        void
        operator()(const Redis_Array& arr)
          {
            this->pfmt->putc('[');
            auto elem_it = arr.begin();
            if(elem_it != arr.end()) {
              // Write the first element without a delimiter.
              auto& this_func = *this;
              elem_it->m_stor.visit(this_func);

              // Write the remaining ones with delimiters.
              while(++ elem_it != arr.end()) {
                this->pfmt->putc(',');
                elem_it->m_stor.visit(this_func);
              }
            }
            this->pfmt->putc(']');
          }
      };

    variant_printer p = { &fmt };
    this->m_stor.visit(p);
    return fmt;
  }

cow_string
Redis_Value::
print_to_string() const
  {
    tinyfmt_str fmt;
    this->print(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
