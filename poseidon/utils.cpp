// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "utils.hpp"
#include "static/logger.hpp"
#include <algorithm>
#include <openssl/rand.h>
#include <openssl/err.h>
#define UNW_LOCAL_ONLY  1
#include <libunwind.h>
namespace poseidon {

bool
do_is_log_enabled(uint8_t level)
  noexcept
  {
    return logger.enabled(level);
  }

bool
do_push_log_message(uint8_t level, const char* func, const char* file, uint32_t line,
                    void* composer, vfn<tinyfmt&, void*>* composer_fn)
  {
    ::rocket::tinyfmt_str fmt;
    (* composer_fn) (fmt, composer);
    cow_string sbuf = fmt.extract_string();
    sbuf.erase(sbuf.rfind_not_of(" \t\r\n") + 1);

    // Enqueue the message.
    logger.enqueue(level, func, file, line, sbuf);
    return true;
  }

::std::runtime_error
do_create_runtime_error(const char* func, const char* file, uint32_t line,
                        void* composer, vfn<tinyfmt&, void*>* composer_fn)
  {
    ::rocket::tinyfmt_str fmt;
    (* composer_fn) (fmt, composer);
    ::std::string sbuf(fmt.c_str(), fmt.length());
    sbuf.erase(sbuf.find_last_not_of(" \t\r\n") + 1);

    // Append the source location and function name.
    ::rocket::ascii_numput nump;
    nump.put_DU(line);
    sbuf += "\n[thrown from function `";
    sbuf += func;
    sbuf += "` at '";
    sbuf += file;
    sbuf += ":";
    sbuf.append(nump.data(), nump.size());
    sbuf += "']";

    ::unw_context_t unw_ctx;
    ::unw_cursor_t unw_top;
    if((::unw_getcontext(&unw_ctx) == 0) && (::unw_init_local(&unw_top, &unw_ctx) == 0)) {
      sbuf += "\n[stack backtrace:";

      // Calculate the number of caller frames.
      size_t nframes = 0;
      ::unw_cursor_t unw_cur = unw_top;
      while(::unw_step(&unw_cur) > 0)
        nframes ++;

      nump.put_DU(nframes);
      static_vector<char, 8> numfield(nump.size(), ' ');

      // Append frames to the exception message.
      nframes = 0;
      unw_cur = unw_top;
      while(::unw_step(&unw_cur) > 0) {
        // * frame index
        nump.put_DU(++ nframes);
        ::std::reverse_copy(nump.begin(), nump.end(), numfield.mut_rbegin());
        sbuf += "\n  ";
        sbuf.append(numfield.data(), numfield.size());
        sbuf += ") ";

        // * instruction pointer
        ::unw_word_t unw_offset;
        ::unw_get_reg(&unw_cur, UNW_REG_IP, &unw_offset);
        nump.put_XU(unw_offset);
        sbuf.append(nump.data(), nump.size());

        char unw_name[1024];
        if(::unw_get_proc_name(&unw_cur, unw_name, sizeof(unw_name), &unw_offset) != 0)
          sbuf += " (unknown)";
        else {
          // * function signature and offset
          sbuf += " `";
          sbuf += unw_name;
          if(unw_offset > 0) {
            sbuf += "`+";
            nump.put_XU(unw_offset);
            sbuf.append(nump.data(), nump.size());
          }
        }
      }

      sbuf += "\n  -- end of stack backtrace]";
    }

    return ::std::runtime_error(sbuf);
  }

void
explode(cow_vector<cow_string>& segments, const cow_string& text, char delim, size_t limit)
  {
    segments.clear();
    size_t bpos = text.find_not_of(" \t");
    while(bpos < text.size()) {
      // Get the end of this segment. If the delimiter is not found, make sure
      // `epos` is reasonably large, so incrementing it will not overflow.
      size_t epos = text.npos / 2;
      if(segments.size() + 1 < limit)
        epos = text.find(bpos, delim) * 2 / 2;

      // Skip trailing blank characters, if any.
      size_t mpos = text.rfind_not_of(epos - 1, " \t");
      ROCKET_ASSERT(mpos != text.npos);
      segments.emplace_back(text.data() + bpos, mpos + 1 - bpos);

      // Skip the delimiter and blank characters that follow it.
      bpos = text.find_not_of(epos + 1, " \t");
    }
  }

cow_vector<cow_string>
explode(const cow_string& text, char delim, size_t limit)
  {
    cow_vector<cow_string> segments;
    explode(segments, text, delim, limit);
    return segments;
  }

void
implode(cow_string& text, const cow_string* segment_ptr, size_t segment_count, char delim)
  {
    text.clear();
    if(segment_count != 0) {
      // Write the first token.
      text << segment_ptr[0];

      // Write the other tokens, each of which is preceded by a delimiter.
      for(size_t k = 1;  k != segment_count;  ++k)
        text << delim << segment_ptr[k];
    }
  }

cow_string
implode(const cow_string* segment_ptr, size_t segment_count, char delim)
  {
    cow_string text;
    implode(text, segment_ptr, segment_count, delim);
    return text;
  }

void
implode(cow_string& text, const cow_vector<cow_string>& segments, char delim)
  {
    implode(text, segments.data(), segments.size(), delim);
  }

cow_string
implode(const cow_vector<cow_string>& segments, char delim)
  {
    cow_string text;
    implode(text, segments, delim);
    return text;
  }

void
quote_json_string(tinyfmt& buf, const cow_string& str)
  {
    buf.putc('"');
    size_t offset = 0;
    while(offset != str.size()) {
      char32_t cp;
      if(!::asteria::utf8_decode(cp, str, offset)) {
        offset ++;
        cp = U'\xFFFD';
      }

      switch(cp)
        {
        case '\b':
          buf.putn("\\b", 2);
          break;

        case '\f':
          buf.putn("\\f", 2);
          break;

        case '\n':
          buf.putn("\\n", 2);
          break;

        case '\r':
          buf.putn("\\r", 2);
          break;

        case '\t':
          buf.putn("\\t", 2);
          break;

        case '\"':  // 0x22
        case '\\':  // 0x5C
          {
            char esc_seq[2] = "\\";
            esc_seq[1] = static_cast<char>(cp);
            buf.putn(esc_seq, 2);
          }
          break;

        case 0x20 ... 0x21:
        case 0x23 ... 0x5B:
        case 0x5D ... 0x7E:
          buf.putc(static_cast<char>(cp));
          break;

        default:
          {
            char16_t ustr[2];
            char16_t* ueptr = ustr;
            ::asteria::utf16_encode(ueptr, cp);

            ::rocket::ascii_numput nump;
            nump.put_XU(ustr[0] * 0x10000UL + ustr[1], 8);

            char esc_seq[16] = "\\uXXXX\\uXXXX";
            ::memcpy(esc_seq + 2, nump.data(), 4);
            ::memcpy(esc_seq + 8, nump.data() + 4, 4);
            buf.putn(esc_seq, static_cast<uint32_t>(ueptr - ustr) * 6);
          }
          break;
        }
    }
    buf.putc('"');
  }

void
hex_encode_16_partial(char* str, const void* data)
  noexcept
  {
    // Split the higher and lower halves into two SSE registers.
    __m128i tval = _mm_loadu_si128(static_cast<const __m128i*>(data));
    __m128i hi = _mm_and_si128(_mm_srli_epi64(tval, 4), _mm_set1_epi8(15));
    __m128i lo = _mm_and_si128(tval, _mm_set1_epi8(15));

    // Convert digits into their string forms:
    //   xdigit := val + '0' + (-(val > 9) & ('a' - 10 - '0'))
    tval = _mm_and_si128(_mm_cmpgt_epi8(hi, _mm_set1_epi8(9)), _mm_set1_epi8(39));
    hi = _mm_add_epi8(_mm_add_epi8(hi, _mm_set1_epi8('0')), tval);
    tval = _mm_and_si128(_mm_cmpgt_epi8(lo, _mm_set1_epi8(9)), _mm_set1_epi8(39));
    lo = _mm_add_epi8(_mm_add_epi8(lo, _mm_set1_epi8('0')), tval);

    // Rearrange digits in the correct order.
    _mm_storeu_si128(reinterpret_cast<__m128i*>(str), _mm_unpacklo_epi8(hi, lo));
    _mm_storeu_si128(reinterpret_cast<__m128i*>(str + 16), _mm_unpackhi_epi8(hi, lo));
    str[32] = 0;
  }

void
random_bytes(void* ptr, size_t size)
  noexcept
  {
    auto ctx = ::OSSL_LIB_CTX_get0_global_default();
    if(!ctx)
      ASTERIA_TERMINATE((
          "Could not get OpenSSL global context: $1",
          "[`OSSL_LIB_CTX_get0_global_default()` failed]"),
          ::ERR_reason_error_string(::ERR_get_error()));

    if(::RAND_bytes_ex(ctx, static_cast<unsigned char*>(ptr), size, 0) != 1)
      ASTERIA_TERMINATE((
          "Could not generate random bytes: $1",
          "[`RAND_bytes_ex()` failed]"),
          ::ERR_reason_error_string(::ERR_get_error()));
  }

size_t
parse_network_reference(Network_Reference& caddr, chars_view str)
  noexcept
  {
    if(str.n == 0)
      return 0;

    // Parse the source string. Users shall have removed leading and trailing
    // whitespace before calling this function.
    const char* bptr;
    const char* mptr = str.p;
    int port_num = 0;

    if(*mptr == '[') {
      // Get an IPv6 address in brackets. An IPv4-mapped address may contain both
      // colons and dots. The address is not otherwise verified.
      bptr = mptr + 1;
      mptr = ::std::find_if_not(bptr, str.p + str.n,
        [](char c) {
          return ((c >= '0') && (c <= '9'))
                 || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                 || (c == ':') || (c == '.');
        });

      if((bptr == mptr) || (mptr == str.p + str.n) || (*mptr != ']'))
        return 0;

      caddr.host.p = bptr;
      caddr.host.n = (size_t) (mptr - bptr);
      caddr.is_ipv6 = true;

      // Skip it.
      ROCKET_ASSERT(*mptr == ']');
      mptr ++;

      if(mptr == str.p + str.n)
        return str.n;
    }
    else {
      // Get a host name or an IPv4 address.
      bptr = mptr;
      mptr = ::std::find_if_not(bptr, str.p + str.n,
        [](char c) {
          return ((c >= '0') && (c <= '9'))
                 || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                 || (c == '-') || (c == '.');
        });

      if(bptr == mptr)
        return 0;

      caddr.host.p = bptr;
      caddr.host.n = (size_t) (mptr - bptr);

      if(mptr == str.p + str.n)
        return str.n;
    }

    if(*mptr == ':') {
      // Get a port number.
      bptr = mptr + 1;
      mptr = ::std::find_if_not(bptr, str.p + str.n,
        [](char c) {
          return ((c >= '0') && (c <= '9'));
        });

      if(bptr == mptr)
        return 0;

      if(!::std::all_of(bptr, mptr,
           [&](char c) {
             port_num = port_num * 10 + (uint8_t) c - '0';
             return port_num <= UINT16_MAX;
           }))
        return 0;

      caddr.port.p = bptr;
      caddr.port.n = (size_t) (mptr - bptr);
      caddr.port_num = (uint16_t) port_num;

      if(mptr == str.p + str.n)
        return str.n;
    }

    if(*mptr == '/') {
      // Get a path. The leading slash is part of the path.
      bptr = mptr;
      mptr = ::std::find_if_not(bptr, str.p + str.n,
        [](char c) {
          return ((c >= '0') && (c <= '9'))
                 || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                 || (c == '-') || (c == '.') || (c == '_') || (c == '~')  // ^^ unreserved
                 || (c == '!') || (c == '$') || (c == '&') || (c == '\'')
                 || (c == '(') || (c == ')') || (c == '*') || (c == '+')
                 || (c == ',') || (c == ';') || (c == '=')  // ^^ sub-delims
                 || (c == '%') || (c == ':') || (c == '@')  // ^^ pchar
                 || (c == '/');
        });

      caddr.path.p = bptr;
      caddr.path.n = (size_t) (mptr - bptr);

      if(mptr == str.p + str.n)
        return str.n;
    }

    if(*mptr == '?') {
      // Get a query string.
      bptr = mptr + 1;
      mptr = ::std::find_if_not(bptr, str.p + str.n,
        [](char c) {
          return ((c >= '0') && (c <= '9'))
                 || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                 || (c == '-') || (c == '.') || (c == '_') || (c == '~')  // ^^ unreserved
                 || (c == '!') || (c == '$') || (c == '&') || (c == '\'')
                 || (c == '(') || (c == ')') || (c == '*') || (c == '+')
                 || (c == ',') || (c == ';') || (c == '=')  // ^^ sub-delims
                 || (c == '%') || (c == ':') || (c == '@')  // ^^ pchar
                 || (c == '/') || (c == '?');
        });

      caddr.query.p = bptr;
      caddr.query.n = (size_t) (mptr - bptr);

      if(mptr == str.p + str.n)
        return str.n;
    }

    if(*mptr == '#') {
      // Get a fragment string.
      bptr = mptr + 1;
      mptr = ::std::find_if_not(bptr, str.p + str.n,
        [](char c) {
          return ((c >= '0') && (c <= '9'))
                 || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                 || (c == '-') || (c == '.') || (c == '_') || (c == '~')  // ^^ unreserved
                 || (c == '!') || (c == '$') || (c == '&') || (c == '\'')
                 || (c == '(') || (c == ')') || (c == '*') || (c == '+')
                 || (c == ',') || (c == ';') || (c == '=')  // ^^ sub-delims
                 || (c == '%') || (c == ':') || (c == '@')  // ^^ pchar
                 || (c == '/') || (c == '?') || (c == '#');
        });

      caddr.fragment.p = bptr;
      caddr.fragment.n = (size_t) (mptr - bptr);

      if(mptr == str.p + str.n)
        return str.n;
    }

    // Return the number of characters that have been consumed.
    return (size_t) (mptr - str.p);
  }

cow_string
decode_and_canonicalize_uri_path(chars_view path)
  {
    cow_string result = &"/";
    const char* bptr = path.p;
    const char* eptr = path.p + path.n;

    auto do_pop_dot_dot = [&](size_t npop)
      {
        result.pop_back(npop);
        ROCKET_ASSERT(result.back() == '/');

        if(result.size() > 1)
          do
            result.pop_back();
          while(result.back() != '/');
      };

    while(bptr != eptr) {
      // Canonicalize once.
      if(result.ends_with("//"))
        result.pop_back(1);
      else if(result.ends_with("/./"))
        result.pop_back(2);
      else if(result.ends_with("/../"))
        do_pop_dot_dot(3);

      uint32_t ch = (uint8_t) *bptr;
      if(ch == '%') {
        // Expect two hexadecimal digits.
        if(eptr - bptr < 3)
          return result;

        if((bptr[1] >= '0') && (bptr[1] <= '9'))
          ch = (uint32_t) ((uint8_t) bptr[1] - '0') << 4;
        else if((bptr[1] >= 'A') && (bptr[1] <= 'F'))
          ch = (uint32_t) ((uint8_t) bptr[1] - 'A' + 10) << 4;
        else if((bptr[1] >= 'a') && (bptr[1] <= 'f'))
          ch = (uint32_t) ((uint8_t) bptr[1] - 'a' + 10) << 4;
        else
          return result;

        if((bptr[2] >= '0') && (bptr[2] <= '9'))
          ch |= (uint32_t) ((uint8_t) bptr[2] - '0');
        else if((bptr[2] >= 'A') && (bptr[2] <= 'F'))
          ch |= (uint32_t) ((uint8_t) bptr[2] - 'A' + 10);
        else if((bptr[2] >= 'a') && (bptr[2] <= 'f'))
          ch |= (uint32_t) ((uint8_t) bptr[2] - 'a' + 10);
        else
          return result;

        // Accept this sequence.
        result += (char) ch;
        bptr += 3;
        continue;
      }

      // Accept this character verbatim.
      result += (char) ch;
      bptr ++;
    }

    // Canonicalize final segment.
    if(result.ends_with("//"))
      result.pop_back(1);
    else if(result.ends_with("/."))
      result.pop_back(1);
    else if(result.ends_with("/./"))
      result.pop_back(2);
    else if(result.ends_with("/.."))
      do_pop_dot_dot(2);
    else if(result.ends_with("/../"))
      do_pop_dot_dot(3);

    ROCKET_ASSERT(result.size() >= 1);
    ROCKET_ASSERT(result[0] == '/');
    return result;
  }

}  // namespace poseidon
