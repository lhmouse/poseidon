// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_value.hpp"
#include "../utils.hpp"
template class POSEIDON_REDIS_VALUE_VARIANT_TYPE_;
namespace poseidon {

Redis_Value::
~Redis_Value()
  {
    // Break deep recursion with a handwritten stack.
    ::std::vector<decltype(Redis_Value::m_stor)> stack;

  do_unpack_loop_:
    switch(this->m_stor.index())
      {
      case redis_value_array:
        try {
          auto& sa = this->m_stor.mut<Redis_Array>();
          if(sa.unique())
            for(auto it = sa.mut_begin();  it != sa.end();  ++it)
              stack.emplace_back().swap(it->m_stor);
        }
        catch(::std::exception& stdex)
          { ::fprintf(stderr, "WARNING: %s\n", stdex.what());  }
        break;
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
print_to(tinyfmt& fmt)
  const
  {
    // Break deep recursion with a handwritten stack.
    struct xFrame
      {
        const Redis_Array* psa;
        Redis_Array::const_iterator ita;
      };

    ::std::vector<xFrame> stack;
    const Redis_Value* pval = this;

  do_unpack_loop_:
    switch(pval->type())
      {
      case redis_value_nil:
        fmt << "nil";
        break;

      case redis_value_integer:
        fmt << pval->as_integer();
        break;

      case redis_value_string:
        fmt.putc('"');
        ::asteria::c_quote(fmt, pval->as_string());
        fmt.putc('"');
        break;

      case redis_value_array:
        if(!pval->as_array().empty()) {
          // open
          auto& frm = stack.emplace_back();
          frm.psa = &(pval->as_array());
          frm.ita = frm.psa->begin();
          fmt << '[';
          pval = &*(frm.ita);
          goto do_unpack_loop_;
        }

        // empty
        fmt << "[]";
        break;

      default:
        ASTERIA_TERMINATE(("Corrupted value type `$1`"), pval->m_stor.index());
      }

    while(!stack.empty()) {
      auto& frm = stack.back();
      if(++ frm.ita != frm.psa->end()) {
        // next
        fmt << ',';
        pval = &*(frm.ita);
        goto do_unpack_loop_;
      }

      // close
      fmt << ']';
      stack.pop_back();
    }

    return fmt;
  }

cow_string
Redis_Value::
to_string()
  const
  {
    tinyfmt_str fmt;
    this->print_to(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
