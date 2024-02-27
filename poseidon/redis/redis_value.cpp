// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_value.hpp"
#include "../utils.hpp"
namespace poseidon {

Redis_Value::
~Redis_Value()
  {
    // Break deep recursion with a handwritten stack.
    struct xVariant : decltype(this->m_stor)  { };
    vector<xVariant> stack;

  do_unpack_loop_:
    try {
      // Unpack arrays or objects.
      auto psa = this->m_stor.mut_ptr<Redis_Array>();
      if(psa && psa->unique())
        for(auto it = psa->mut_begin();  it != psa->end();  ++it)
          stack.emplace_back().swap(it->m_stor);
    }
    catch(::std::exception& stdex) {
      // Ignore this exception.
      ::std::fprintf(stderr, "WARNING: %s\n", stdex.what());
    }

    if(!stack.empty()) {
      // Destroy the this value. This will not result in recursion.
      ::rocket::destroy(&(this->m_stor));
      ::rocket::construct(&(this->m_stor), move(stack.back()));
      stack.pop_back();
      goto do_unpack_loop_;
    }
  }

tinyfmt&
Redis_Value::
print_to(tinyfmt& fmt) const
  {
    struct variant_printer
      {
        tinyfmt* pfmt;

        void
        operator()(nullptr_t)
          {
            this->pfmt->putn("(nil)", 5);
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
            auto pos = str.begin();
            while(pos != str.end()) {
              // Escape 'common' characters, like in JSON.
              //                                    ' " \  \a  \b  \v  \f  \n  \r  \t  \0
              static constexpr char src_chars[] = "\'\"\\""\a""\b""\v""\f""\n""\r""\t";
              static constexpr char esc_chars[] = "\'\"\\" "a" "b" "v" "f" "n" "r" "t" "0";
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
    this->print_to(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
