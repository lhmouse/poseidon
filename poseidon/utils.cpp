// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "precompiled.ipp"
#include "utils.hpp"
#include <libunwind.h>
#include <openssl/rand.h>
namespace poseidon {

void
throw_runtime_error_with_backtrace(const char* file, long line, const char* func, cow_string&& msg)
  {
    // Remove trailing space characters from the message.
    size_t pos = msg.rfind_not_of(" \f\n\r\t\v");
    msg.erase(pos + 1);

    // Calculate the number of stack frames.
    size_t nframes = 0;
    ::unw_context_t unw_ctx[1];
    ::unw_cursor_t unw_cur[1];
    char unw_name[1024];
    ::unw_word_t unw_offset;

    if((::unw_getcontext(unw_ctx) == 0) && (::unw_init_local(unw_cur, unw_ctx) == 0))
      while(::unw_step(unw_cur) > 0)
        ++ nframes;

    // Determine the width of the frame index field.
    ::rocket::ascii_numput nump;
    nump.put_DU(nframes);
    static_vector<char, 8> numfield(nump.size(), ' ');
    numfield.emplace_back();

    // Compose the string to throw.
    tinyfmt_ln fmt;
    fmt << func << ": " << msg;
    fmt << "\n[thrown from '" << file << ':' << line << "']";

    if(nframes != 0) {
      fmt << "\n[stack backtrace:";
      nframes = 0;

      if(::unw_init_local(unw_cur, unw_ctx) == 0)
        while(::unw_step(unw_cur) > 0) {
          // * frame index
          nump.put_DU(++ nframes);
          ::std::reverse_copy(nump.begin(), nump.end(), numfield.mut_rbegin() + 1);
          fmt << "\n  " << numfield.data() << ") ";

          // * instruction pointer
          ::unw_get_reg(unw_cur, UNW_REG_IP, &unw_offset);
          nump.put_XU(unw_offset);
          fmt << nump << " ";

          // * function name and offset
          if(::unw_get_proc_name(unw_cur, unw_name, sizeof(unw_name), &unw_offset) == 0) {
            nump.put_XU(unw_offset);
            fmt << "`" << unw_name << "` +" << nump;
          } else
            fmt << "??";
        }

      fmt << "\n  -- end of stack backtrace]";
    }
    else
      fmt << "\n[no stack backtrace available]";

    // Throw it.
    fmt << '\0';
    throw ::std::runtime_error(fmt.data());
  }

cow_string
ascii_uppercase(cow_string text)
  {
    // Only modify the string when it really has to modified.
    for(size_t k = 0;  k != text.size();  ++k) {
      char32_t ch = (uint8_t) text[k];
      if(('a' <= ch) && (ch <= 'z'))
        text.mut(k) = (char) (ch - 0x20);
    }
    return ::std::move(text);
  }

cow_string
ascii_lowercase(cow_string text)
  {
    // Only modify the string when it really has to modified.
    for(size_t k = 0;  k != text.size();  ++k) {
      char32_t ch = (uint8_t) text[k];
      if(('A' <= ch) && (ch <= 'Z'))
        text.mut(k) = (char) (ch + 0x20);
    }
    return ::std::move(text);
  }

cow_string
ascii_trim(cow_string text)
  {
    // Remove leading blank characters.
    // Return an empty string if all characters are blank.
    size_t k = cow_string::npos;
    for(;;) {
      if(++k == text.size())
        return { };

      char32_t ch = (uint8_t) text[k];
      if((ch != ' ') && (ch != '\t'))
        break;
    }
    if(k != 0)
      text.erase(0, k);

    // Remove trailing blank characters.
    k = text.size();
    for(;;) {
      if(--k == 0)
        break;

      char32_t ch = (uint8_t) text[k];
      if((ch != ' ') && (ch != '\t'))
        break;
    }
    if(++k != text.size())
      text.erase(k);

    return ::std::move(text);
  }

size_t
explode(cow_vector<cow_string>& segments, cow_stringR text, char delim, size_t limit)
  {
    segments.clear();
    size_t bpos = text.find_not_of(" \t");
    while(bpos < text.size()) {
      // Get the end of this segment.
      // If the delimiter is not found, make sure `epos` is reasonably large
      // and incrementing it will not overflow.
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
    return segments.size();
  }

size_t
implode(cow_string& text, const cow_vector<cow_string>& segments, char delim)
  {
    text.clear();
    if(segments.size()) {
      // Write the first token.
      text << segments[0];

      // Write the other tokens, each of which is preceded by a delimiter.
      for(size_t k = 1;  k < segments.size();  ++k)
        text << delim << ' ' << segments[k];
    }
    return segments.size();
  }

char*
hex_encode_16_partial(char* str, const void* data) noexcept
  {
    // Split the higher and lower halves into two SSE registers.
    __m128i tval = _mm_loadu_si128((const __m128i*) data);
    __m128i hi = _mm_and_si128(_mm_srli_epi64(tval, 4), _mm_set1_epi8(15));
    __m128i lo = _mm_and_si128(tval, _mm_set1_epi8(15));

    // Convert digits into their string forms:
    //   xdigit := val + '0' + ((val > 9) ? 7 : 0)
    tval = _mm_and_si128(_mm_cmpgt_epi8(hi, _mm_set1_epi8(9)), _mm_set1_epi8(39));
    hi = _mm_add_epi8(_mm_add_epi8(hi, _mm_set1_epi8('0')), tval);

    tval = _mm_and_si128(_mm_cmpgt_epi8(lo, _mm_set1_epi8(9)), _mm_set1_epi8(39));
    lo = _mm_add_epi8(_mm_add_epi8(lo, _mm_set1_epi8('0')), tval);

    // Rearrange digits in the correct order.
    _mm_storeu_si128((__m128i*) str, _mm_unpacklo_epi8(hi, lo));
    _mm_storeu_si128((__m128i*) (str + 16), _mm_unpackhi_epi8(hi, lo));
    str[32] = 0;
    return str;
  }

uint32_t
random_uint32() noexcept
  {
    uint32_t bits;
    ::RAND_bytes((uint8_t*) &bits, sizeof(bits));
    return bits;
  }

uint64_t
random_uint64() noexcept
  {
    uint64_t bits;
    ::RAND_bytes((uint8_t*) &bits, sizeof(bits));
    return bits;
  }

float
random_float() noexcept
  {
    uint32_t bits;
    ::RAND_bytes((uint8_t*) &bits, sizeof(bits));
    bits = 0x7FU << 23 | bits >> 9;  // 1:8:23
    float valp1;
    ::memcpy(&valp1, &bits, sizeof(valp1));
    return valp1 - 1;
  }

double
random_double() noexcept
  {
    uint64_t bits;
    ::RAND_bytes((uint8_t*) &bits, sizeof(bits));
    bits = 0x3FFULL << 52 | bits >> 12;  // 1:11:52
    double valp1;
    ::memcpy(&valp1, &bits, sizeof(valp1));
    return valp1 - 1;
  }

size_t
parse_network_reference(Network_Reference& caddr, chars_view str) noexcept
  {
    // Set fields to default values.
    caddr = Network_Reference();

    // Parse the source string. Users shall have removed leading and trailing
    // whitespace before calling this function.
    if(str.n == 0)
      return 0;

    const char* bptr;
    const char* mptr = str.p;
    uint32_t nport = 0;

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

      for(auto q = bptr;  q != mptr;  ++q)
        if((nport = nport * 10 + (uint8_t) *q - '0') > UINT16_MAX)
          return 0;

      caddr.port.p = bptr;
      caddr.port.n = (size_t) (mptr - bptr);
      caddr.port_num = (uint16_t) nport;

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

}  // namespace poseidon
