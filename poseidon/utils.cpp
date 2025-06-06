// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "utils.hpp"
#include <openssl/rand.h>
#include <openssl/err.h>
#define UNW_LOCAL_ONLY  1
#include <libunwind.h>
namespace poseidon {

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
quote_json_string(tinybuf& buf, const cow_string& str)
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
quote_json_string(tinyfmt& fmt, const cow_string& str)
  {
    quote_json_string(fmt.mut_buf(), str);
  }

void
hex_encode_16_partial(char* str, const void* data) noexcept
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
random_bytes(void* ptr, size_t size) noexcept
  {
    auto ctx = ::OSSL_LIB_CTX_get0_global_default();
    if(!ctx)
      ASTERIA_TERMINATE((
          "Could not get OpenSSL global context: $1",
          "[`OSSL_LIB_CTX_get0_global_default()` failed]"),
          ::ERR_reason_error_string(::ERR_peek_error()));

    if(::RAND_bytes_ex(ctx, static_cast<unsigned char*>(ptr), size, 0) != 1)
      ASTERIA_TERMINATE((
          "Could not generate random bytes: $1",
          "[`RAND_bytes()` failed]"),
          ::ERR_reason_error_string(::ERR_peek_error()));
  }

size_t
parse_network_reference(Network_Reference& caddr, chars_view str) noexcept
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

bool
enqueue_log_message(void composer_callback(cow_string&, void*), void* composer,
                    uint8_t level, const char* func, const char* file,
                    uint32_t line) noexcept
  try {
    cow_string sbuf;
    composer_callback(sbuf, composer);
    sbuf.erase(sbuf.rfind_not_of(" \t\r\n") + 1);

    // Enqueue the message.
    logger.enqueue(level, func, file, line, sbuf);
    return true;
  }
  catch(exception& stdex) {
    ::fprintf(stderr, "WARNING: %s\n", stdex.what());
    return false;
  }

::std::runtime_error
create_runtime_error(void composer_callback(cow_string&, void*), void* composer,
                     const char* func, const char* file, uint32_t line)
  try {
    cow_string tbuf;
    composer_callback(tbuf, composer);
    ::std::string sbuf(tbuf.data(), tbuf.rfind_not_of(" \t\r\n") + 1);

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

      // Don't leak the buffer that will be returned by `__cxa_demangle()`.
      ::rocket::unique_ptr<char, void (void*)> demangle_buf(nullptr, ::free);
      size_t demangle_size = 0;

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
          // Demangle the function name. If `__cxa_demangle()` returns a
          // non-null pointer, `demangle_buf` will have been reallocated to
          // `fn` which will point to the demangled name.
          char* fn = ::abi::__cxa_demangle(unw_name, demangle_buf, &demangle_size, nullptr);
          if(!fn)
            fn = unw_name;
          else {
            demangle_buf.release();
            demangle_buf.reset(fn);
          }

          // * function name and offset
          sbuf += " `";
          sbuf += fn;
          sbuf += "` +";
          nump.put_XU(unw_offset);
          sbuf.append(nump.data(), nump.size());
        }
      }

      sbuf += "\n  -- end of stack backtrace]";
    }

    return ::std::runtime_error(sbuf);
  }
  catch(::std::runtime_error& stdex) {
    ::fprintf(stderr, "WARNING: %s\n", stdex.what());
    return move(stdex);
  }
  catch(exception& stdex) {
    ::fprintf(stderr, "WARNING: %s\n", stdex.what());
    return ::std::runtime_error(stdex.what());
  }

}  // namespace poseidon
