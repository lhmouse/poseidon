// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "uuid.hpp"
#include "../utils.hpp"
#include <openssl/rand.h>
#include <openssl/err.h>
namespace poseidon {

const uuid uuid_nil = (uuid::fields) { 0x00000000,0x0000,0x0000,0x0000,0x000000000000 };
const uuid uuid_max = (uuid::fields) { 0xFFFFFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFFFFFFFFFF };

uuid::
uuid(const random&)
  {
    // Get system information.
    static atomic<uint64_t> s_count;
    ::timespec tv;
    ::clock_gettime(CLOCK_REALTIME, &tv);
    uint64_t time = (uint64_t) tv.tv_sec * 30518U + (uint32_t) tv.tv_nsec / 32768U + s_count.xadd(1U);
    uint32_t version_pid = (uint32_t) ::getpid();

    uint64_t variant_random;
    if(::RAND_priv_bytes((unsigned char*) &variant_random, sizeof(variant_random)) != 1)
      POSEIDON_THROW((
          "Could not generate random bytes",
          "[`RAND_priv_bytes()` failed: $1]"),
          ::ERR_reason_error_string(::ERR_peek_error()));

    // Set the UUID version to `4` and the UUID variant to `6`.
    version_pid = 0x4000U | (version_pid & 0x0FFFU);
    variant_random = 0x60000000'00000000U | (variant_random >> 3);

    // Fill UUID fields.
    this->m_data_1_3 = (uint8_t) (time >> 40);
    this->m_data_1_2 = (uint8_t) (time >> 32);
    this->m_data_1_1 = (uint8_t) (time >> 24);
    this->m_data_1_0 = (uint8_t) (time >> 16);
    this->m_data_2_1 = (uint8_t) (time >>  8);
    this->m_data_2_0 = (uint8_t)  time;
    this->m_data_3_1 = (uint8_t) (version_pid >> 8);
    this->m_data_3_0 = (uint8_t)  version_pid;
    this->m_data_4_1 = (uint8_t) (variant_random >> 56);
    this->m_data_4_0 = (uint8_t) (variant_random >> 48);
    this->m_data_5_5 = (uint8_t) (variant_random >> 40);
    this->m_data_5_4 = (uint8_t) (variant_random >> 32);
    this->m_data_5_3 = (uint8_t) (variant_random >> 24);
    this->m_data_5_2 = (uint8_t) (variant_random >> 16);
    this->m_data_5_1 = (uint8_t) (variant_random >>  8);
    this->m_data_5_0 = (uint8_t)  variant_random;
  }

uuid::
uuid(const char* str, size_t len)
  {
    size_t r = this->parse(str, len);
    if(r != len)
      POSEIDON_THROW((
          "Could not parse UUID string `$1`"),
          cow_string(str, len));
  }

uuid::
uuid(const char* str)
  {
    size_t r = this->parse(str, ::strlen(str));
    if(str[r] != 0)
      POSEIDON_THROW((
          "Could not parse UUID string `$1`"),
          str);
  }

uuid::
uuid(cow_stringR str)
  {
    size_t r = this->parse(str.data(), str.size());
    if(r != str.size())
      POSEIDON_THROW((
          "Could not parse UUID string `$1`"),
          str);
  }

int
uuid::
compare(const uuid& other) const noexcept
  {
    return ::memcmp(&(this->m_stor), &(other.m_stor), 16);
  }

size_t
uuid::
parse_partial(const char* str) noexcept
  {
#define do_match_xdigit_hi_(di, si)  \
    if((str[si] >= '0') && (str[si] <= '9'))  \
      di = (uint8_t) ((str[si] - '0') << 4);  \
    else if((str[si] >= 'A') && (str[si] <= 'F'))  \
      di = (uint8_t) ((str[si] - 'A' + 10) << 4);  \
    else if((str[si] >= 'a') && (str[si] <= 'f'))  \
      di = (uint8_t) ((str[si] - 'a' + 10) << 4);  \
    else  \
      return 0;

#define do_match_xdigit_lo_(di, si)  \
    if((str[si] >= '0') && (str[si] <= '9'))  \
      di |= (uint8_t) (str[si] - '0');  \
    else if((str[si] >= 'A') && (str[si] <= 'F'))  \
      di |= (uint8_t) (str[si] - 'A' + 10);  \
    else if((str[si] >= 'a') && (str[si] <= 'f'))  \
      di |= (uint8_t) (str[si] - 'a' + 10);  \
    else  \
      return 0;

#define do_match_dash_(si)  \
    if(str[si] != '-')  \
      return 0;

    do_match_xdigit_hi_ (this->m_data_1_3,  0)
    do_match_xdigit_lo_ (this->m_data_1_3,  1)
    do_match_xdigit_hi_ (this->m_data_1_2,  2)
    do_match_xdigit_lo_ (this->m_data_1_2,  3)
    do_match_xdigit_hi_ (this->m_data_1_1,  4)
    do_match_xdigit_lo_ (this->m_data_1_1,  5)
    do_match_xdigit_hi_ (this->m_data_1_0,  6)
    do_match_xdigit_lo_ (this->m_data_1_0,  7)
    do_match_dash_      (                   8)
    do_match_xdigit_hi_ (this->m_data_2_1,  9)
    do_match_xdigit_lo_ (this->m_data_2_1, 10)
    do_match_xdigit_hi_ (this->m_data_2_0, 11)
    do_match_xdigit_lo_ (this->m_data_2_0, 12)
    do_match_dash_      (                  13)
    do_match_xdigit_hi_ (this->m_data_3_1, 14)
    do_match_xdigit_lo_ (this->m_data_3_1, 15)
    do_match_xdigit_hi_ (this->m_data_3_0, 16)
    do_match_xdigit_lo_ (this->m_data_3_0, 17)
    do_match_dash_      (                  18)
    do_match_xdigit_hi_ (this->m_data_4_1, 19)
    do_match_xdigit_lo_ (this->m_data_4_1, 20)
    do_match_xdigit_hi_ (this->m_data_4_0, 21)
    do_match_xdigit_lo_ (this->m_data_4_0, 22)
    do_match_dash_      (                  23)
    do_match_xdigit_hi_ (this->m_data_5_5, 24)
    do_match_xdigit_lo_ (this->m_data_5_5, 25)
    do_match_xdigit_hi_ (this->m_data_5_4, 26)
    do_match_xdigit_lo_ (this->m_data_5_4, 27)
    do_match_xdigit_hi_ (this->m_data_5_3, 28)
    do_match_xdigit_lo_ (this->m_data_5_3, 29)
    do_match_xdigit_hi_ (this->m_data_5_2, 30)
    do_match_xdigit_lo_ (this->m_data_5_2, 31)
    do_match_xdigit_hi_ (this->m_data_5_1, 32)
    do_match_xdigit_lo_ (this->m_data_5_1, 33)
    do_match_xdigit_hi_ (this->m_data_5_0, 34)
    do_match_xdigit_lo_ (this->m_data_5_0, 35)

    // Return the number of characters that have been accepted, which
    // is always `36` for this function.
    return 36;
  }

size_t
uuid::
parse(const char* str, size_t len) noexcept
  {
    // A string with an erroneous length will not be accepted, so
    // we just need to check for possibilities by `len`.
    size_t acc_len = 0;

    if(len >= 36)
      acc_len = this->parse_partial(str);

    return acc_len;
  }

size_t
uuid::
print_partial(char* str) const noexcept
  {
    // These hacks for GCC 10- shall be removed in the future.
#if defined(__GNUC__) && (__GNUC__ < 11)
#  define _mm_storeu_si64(ptr, val)   _mm_store_sd((double*) (ptr), _mm_castsi128_pd(val))
#  define _mm_storeu_si32(ptr, val)   _mm_store_ss((float*) (ptr), _mm_castsi128_ps(val))
#endif
    __m128i tval, hi, lo;

    // Split the higher and lower halves into two SSE registers.
    tval = _mm_loadu_si128((const __m128i*) &(this->m_stor));
    hi = _mm_and_si128(_mm_srli_epi64(tval, 4), _mm_set1_epi8(0x0F));
    lo = _mm_and_si128(tval, _mm_set1_epi8(0x0F));

    // Convert digits into their string forms:
    //   xdigit := val + '0' + ((val > 9) ? 7 : 0)
    tval = _mm_and_si128(_mm_cmpgt_epi8(hi, _mm_set1_epi8(9)), _mm_set1_epi8(7));
    hi = _mm_add_epi8(_mm_add_epi8(hi, _mm_set1_epi8('0')), tval);

    tval = _mm_and_si128(_mm_cmpgt_epi8(lo, _mm_set1_epi8(9)), _mm_set1_epi8(7));
    lo = _mm_add_epi8(_mm_add_epi8(lo, _mm_set1_epi8('0')), tval);

    // Insert dashes first. Instead of writing four dashes into `str[8]`,
    // `str[13]`, `str[18]` and `str[23]`, we can overwrite that 16-byte
    // range with a single store operation.
    _mm_storeu_si128((__m128i*) (str + 8), _mm_set1_epi8('-'));

    tval = _mm_unpacklo_epi8(hi, lo);
    _mm_storeu_si64(str, tval);
    _mm_storeu_si32(str + 9, _mm_bsrli_si128(tval, 8));
    _mm_storeu_si32(str + 14, _mm_bsrli_si128(tval, 12));

    tval = _mm_unpackhi_epi8(hi, lo);
    _mm_storeu_si32(str + 19, tval);
    _mm_storeu_si32(str + 24, _mm_bsrli_si128(tval, 4));
    _mm_storeu_si64(str + 28, _mm_bsrli_si128(tval, 8));

    // Return the number of characters that have been written, which is
    // always `36` for this function.
    return 36;
  }

tinyfmt&
uuid::
print(tinyfmt& fmt) const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return fmt.putn(str, len);
  }

cow_string
uuid::
print_to_string() const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
