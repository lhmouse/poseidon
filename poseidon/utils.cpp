// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "utils.hpp"
#include <openssl/rand.h>
namespace poseidon {

cow_string
ascii_uppercase(cow_string text)
  {
    // Only modify the string when it really has to modified.
    for(size_t k = 0;  k != text.size();  ++k) {
      char32_t ch = (uint8_t) text[k];
      if(('a' <= ch) && (ch <= 'z'))
        text.mut(k) = (char) (ch - 0x20);
    }
    return move(text);
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
    return move(text);
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

    return move(text);
  }

void
explode(vector<cow_string>& segments, cow_stringR text, char delim, size_t limit)
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
  }

vector<cow_string>
explode(cow_stringR text, char delim, size_t limit)
  {
    vector<cow_string> segments;
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
implode(cow_string& text, const vector<cow_string>& segments, char delim)
  {
    implode(text, segments.data(), segments.size(), delim);
  }

cow_string
implode(const vector<cow_string>& segments, char delim)
  {
    cow_string text;
    implode(text, segments, delim);
    return text;
  }

void
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

}  // namespace poseidon
