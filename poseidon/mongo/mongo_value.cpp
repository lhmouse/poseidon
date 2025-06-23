// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_value.hpp"
#include "../utils.hpp"
#define OPENSSL_API_COMPAT  0x10100000L
#include <openssl/evp.h>
template class POSEIDON_MONGO_VALUE_VARIANT_TYPE_;
namespace poseidon {

Mongo_Value::
~Mongo_Value()
  {
    // Break deep recursion with a handwritten stack.
    ::std::vector<decltype(Mongo_Value::m_stor)> stack;

  do_unpack_loop_:
    try {
      // Unpack arrays or objects.
      auto psa = this->m_stor.mut_ptr<Mongo_Array>();
      if(psa && psa->unique())
        for(auto it = psa->mut_begin();  it != psa->end();  ++it)
          stack.emplace_back().swap(it->m_stor);

      auto pso = this->m_stor.mut_ptr<Mongo_Document>();
      if(pso && pso->unique())
        for(auto it = pso->mut_begin();  it != pso->end();  ++it)
          stack.emplace_back().swap(it->second.m_stor);
    }
    catch(::std::exception& stdex) {
      // Ignore this exception.
      ::fprintf(stderr, "WARNING: %s\n", stdex.what());
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
Mongo_Value::
print_to(tinyfmt& fmt) const
  {
    // Break deep recursion with a handwritten stack.
    struct xFrame
      {
        char closure;
        const Mongo_Array* psa;
        Mongo_Array::const_iterator ita;
        const Mongo_Document* pso;
        Mongo_Document::const_iterator ito;
      };

    ::std::vector<xFrame> stack;
    const Mongo_Value* pval = this;

  do_unpack_loop_:
    switch(pval->type())
      {
      case mongo_value_null:
        fmt << "null";
        break;

      case mongo_value_boolean:
        fmt << pval->as_boolean();
        break;

      case mongo_value_integer:
        fmt << pval->as_integer();
        break;

      case mongo_value_double:
        fmt << pval->as_double();
        break;

      case mongo_value_utf8:
        quote_json_string(fmt, pval->as_utf8());
        break;

      case mongo_value_binary:
        {
          fmt.putn("{$base64:\"", 10);
          size_t offset = 0;
          while(offset < pval->as_binary_size()) {
            // Encode source data in blocks.
            unsigned char output[68];
            uint32_t bsize = clamp_cast<uint32_t>(pval->as_binary_size() - offset, 0, 48);
            ::EVP_EncodeBlock(output, pval->as_binary_data() + offset, static_cast<int>(bsize));
            fmt.putn(reinterpret_cast<const char*>(output), (bsize + 2) / 3 * 4);
            offset += bsize;
          }
          fmt.putn("\"}", 2);
        }
        break;

      case mongo_value_array:
        if(!pval->as_array().empty()) {
          // open
          auto& frm = stack.emplace_back();
          frm.closure = ']';
          frm.psa = &(pval->as_array());
          frm.ita = frm.psa->begin();
          fmt << '[';
          pval = &*(frm.ita);
          goto do_unpack_loop_;
        }

        // empty
        fmt << "[]";
        break;

      case mongo_value_document:
        if(!pval->as_document().empty()) {
          // open
          auto& frm = stack.emplace_back();
          frm.closure = '}';
          frm.pso = &(pval->as_document());
          frm.ito = frm.pso->begin();
          fmt << '{';
          quote_json_string(fmt, frm.ito->first);
          fmt << ':';
          pval = &(frm.ito->second);
          goto do_unpack_loop_;
        }

        // empty
        fmt << "{}";
        break;

      case mongo_value_oid:
        {
          // `{$oid:"00112233445566778899aabb"}`
          char temp[40];
          ::memcpy(temp, "{$oid:\"", 8);
          ::bson_oid_to_string(&(pval->as_oid()), temp + 7);
          ::memcpy(temp + 31, "\"}\0", 4);
          fmt << temp;
        }
        break;

      case mongo_value_datetime:
        {
          // `{$date:"1994-11-06T08:49:37.123Z}"}`
          char temp[40];
          ::memcpy(temp, "{$date:\"", 8);
          pval->as_datetime().print_iso8601_ns_partial(temp + 8);
          ::memcpy(temp + 31, "Z\"}", 4);
          fmt.putn(temp, 34);
        }
        break;

      default:
        ASTERIA_TERMINATE(("Corrupted value type `$1`"), pval->m_stor.index());
      }

    while(!stack.empty()) {
      auto& frm = stack.back();
      switch(frm.closure)
        {
        case ']':
          if(++ frm.ita != frm.psa->end()) {
            // next
            fmt << ',';
            pval = &*(frm.ita);
            goto do_unpack_loop_;
          }
          break;

        case '}':
          if(++ frm.ito != frm.pso->end()) {
            // next
            fmt << ',';
            quote_json_string(fmt, frm.ito->first);
            fmt << ':';
            pval = &(frm.ito->second);
            goto do_unpack_loop_;
          }
          break;

        default:
          ROCKET_UNREACHABLE();
        }

      // close
      fmt << frm.closure;
      stack.pop_back();
    }

    return fmt;
  }

cow_string
Mongo_Value::
to_string() const
  {
    tinyfmt_str fmt;
    this->print_to(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
