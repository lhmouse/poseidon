// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "uuid.hpp"
#include "../utils.hpp"
#include <openssl/rand.h>
#include <openssl/err.h>
namespace poseidon {
namespace {

constexpr UUID s_min = POSEIDON_UUID(00000000,0000,0000,0000,000000000000);
constexpr UUID s_max = POSEIDON_UUID(ffffffff,ffff,ffff,ffff,ffffffffffff);

atomic_relaxed<uint64_t> s_serial;

}  // namespace

UUID::
UUID(chars_view str)
  {
    if(this->parse(str) != str.n)
      POSEIDON_THROW(("Could not parse UUID string `$1`"), str);
  }

const UUID&
UUID::
min()
  noexcept
  {
    return s_min;
  }

const UUID&
UUID::
max()
  noexcept
  {
    return s_max;
  }

UUID
UUID::
random()
  noexcept
  {
    UUID result;

    struct timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t ts_pid = (static_cast<uint64_t>(ts.tv_sec - 983404800) * 30518 << 16)
                      + (static_cast<uint32_t>(ts.tv_nsec) / 32768 << 16)
                      + (s_serial.xadd(1) << 16)
                      + 0x4000
                      + static_cast<uint32_t>(::getpid() & 0x0FFF);

    uint64_t random;
    if(::RAND_bytes(reinterpret_cast<unsigned char*>(&random), 8) != 1)
      ASTERIA_TERMINATE((
          "Could not generate random bytes: $1",
          "[`RAND_bytes()` failed]"),
          ::ERR_reason_error_string(::ERR_get_error()));

    ::rocket::store_be<uint64_t>(result.m_bytes.data() + 0, ts_pid);
    ::rocket::store_be<uint64_t>(result.m_bytes.data() + 8, random >> 1);
    return result;
  }

int
UUID::
compare(const UUID& other)
  const noexcept
  {
    __m128i bswap = _mm_setr_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
    __m128i shift = _mm_set1_epi8(-128);
    __m128i tval = _mm_xor_si128(_mm_shuffle_epi8(this->m_stor, bswap), shift);
    __m128i oval = _mm_xor_si128(_mm_shuffle_epi8(other.m_stor, bswap), shift);

    int gt = _mm_movemask_epi8(_mm_cmpgt_epi8(tval, oval));
    int lt = _mm_movemask_epi8(_mm_cmpgt_epi8(oval, tval));
    return gt - lt;
  }

size_t
UUID::
parse_partial(const char* str)
  noexcept
  {
    uint64_t high = 0;
    uint64_t low = 0;

    for(uint32_t k = 0;  k != 36;  ++k)
      if(1ULL << k & 0b100001000010000100000000ULL) {
        // dash
        if(str[k] != '-')
          return 0;
      }
      else {
        // hex
        uint64_t val;
        if((str[k] >= '0') && (str[k] <= '9'))
          val = static_cast<unsigned char>(str[k] - '0');
        else if((str[k] >= 'A') && (str[k] <= 'F'))
          val = static_cast<unsigned char>(str[k] - 'A' + 10);
        else if((str[k] >= 'a') && (str[k] <= 'f'))
          val = static_cast<unsigned char>(str[k] - 'a' + 10);
        else
          return 0;

        if(k < 19)
          high = high << 4 | val;
        else
          low = low << 4 | val;
      }

    ::rocket::store_be<uint64_t>(this->m_bytes.data() + 0, high);
    ::rocket::store_be<uint64_t>(this->m_bytes.data() + 8, low);
    return 36;
  }

size_t
UUID::
parse(chars_view str)
  noexcept
  {
    if(str.n >= 36)
      if(size_t aclen = this->parse_partial(str.p))
        return aclen;

    return 0;
  }

size_t
UUID::
print_partial(char* str)
  const noexcept
  {
    //            0123456789012345678901234567890123456
    // * str    : xxxxxxxx-xxxx-Myyy-Nzzz-zzzzzzzzzzzz.
    // * hexstr : xxxxxxxxxxxxMyyyNzzzzzzzzzzzzzzz.
    char hexstr[33];
    hex_encode_16_partial(hexstr, this->m_bytes.data());

    str[36] = 0;
    ::memcpy(str + 20, hexstr + 16, 16);
    str[23] = '-';
    ::memcpy(str + 19, hexstr + 16,  4);
    str[18] = '-';
    ::memcpy(str + 14, hexstr + 12,  4);
    str[13] = '-';
    ::memcpy(str +  9, hexstr +  8,  4);
    str[ 8] = '-';
    ::memcpy(str +  0, hexstr +  0,  8);
    return 36;
  }

tinyfmt&
UUID::
print_to(tinyfmt& fmt)
  const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return fmt.putn(str, len);
  }

cow_string
UUID::
to_string()
  const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
