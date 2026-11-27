// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_IPV6_ADDRESS_
#define POSEIDON_SOCKET_IPV6_ADDRESS_

#include "../fwd.hpp"
#include "enums.hpp"
#include <netinet/in.h>
namespace poseidon {

class IPv6_Address
  {
  public:
    struct hash;

    struct fields
      {
        uint16_t addr_h[6];
        uint8_t addr_l[4];
        uint16_t port;
      };

  private:
    union {
      ::std::array<uint8_t, 16> m_bytes = { };
      ::in6_addr m_addr;
      __m128i m_addr_stor;
    };
    uint16_t m_port = 0;

  public:
    // Initializes an unspecified (all-zero) address.
    constexpr IPv6_Address() noexcept = default;

    // Constructs an address from individual fields.
    constexpr IPv6_Address(const fields& f) noexcept
      {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
        this->m_bytes[0] = f.addr_h[0] >> 8;
        this->m_bytes[1] = f.addr_h[0];
        this->m_bytes[2] = f.addr_h[1] >> 8;
        this->m_bytes[3] = f.addr_h[1];
        this->m_bytes[4] = f.addr_h[2] >> 8;
        this->m_bytes[5] = f.addr_h[2];
        this->m_bytes[6] = f.addr_h[3] >> 8;
        this->m_bytes[7] = f.addr_h[3];
        this->m_bytes[8] = f.addr_h[4] >> 8;
        this->m_bytes[9] = f.addr_h[4];
        this->m_bytes[10] = f.addr_h[5] >> 8;
        this->m_bytes[11] = f.addr_h[5];
        this->m_bytes[12] = f.addr_l[0];
        this->m_bytes[13] = f.addr_l[1];
        this->m_bytes[14] = f.addr_l[2];
        this->m_bytes[15] = f.addr_l[3];
#pragma GCC diagnostic pop
        this->m_port = f.port;
      }

    // Initializes an address from a foreign source.
    constexpr IPv6_Address(const ::in6_addr& addr, uint16_t port) noexcept
      :
        m_addr(addr), m_port(port)
      { }

    constexpr IPv6_Address(const IPv6_Address& other, uint16_t port) noexcept
      :
        m_addr(other.m_addr), m_port(port)
      { }

    // Parses an address from a string, like `parse()`.
    // An exception is thrown if the address string is not valid.
    explicit IPv6_Address(chars_view str);

    IPv6_Address&
    swap(IPv6_Address& other) noexcept
      {
        ::std::swap(this->m_addr, other.m_addr);
        ::std::swap(this->m_port, other.m_port);
        return *this;
      }

  public:
    // Accesses raw data.
    constexpr
    const ::in6_addr&
    addr() const noexcept
      { return this->m_addr;  }

    constexpr
    uint16_t
    port() const noexcept
      { return this->m_port;  }

    const uint8_t*
    data() const noexcept
      { return (const uint8_t*) &(this->m_addr);  }

    uint8_t*
    mut_data() noexcept
      { return (uint8_t*) &(this->m_addr);  }

    ::in6_addr&
    mut_addr() noexcept
      { return this->m_addr;  }

    void
    set_addr(const ::in6_addr& addr) noexcept
      { this->m_addr = addr;  }

    void
    set_port(uint16_t port) noexcept
      { this->m_port = port;  }

    void
    clear() noexcept
      {
        this->m_addr = ::in6_addr();
        this->m_port = 0;
      }

    // Performs bitwise comparison.
    ASTERIA_PURE
    bool
    equals(const IPv6_Address& other) const noexcept
      {
        __m128i tval = _mm_load_si128(&(this->m_addr_stor));
        __m128i oval = _mm_load_si128(&(other.m_addr_stor));
        int mask_ne = _mm_movemask_epi8(_mm_cmpeq_epi16(tval, oval)) ^ 0xFFFF;
        mask_ne |= this->m_port ^ other.m_port;
        return mask_ne == 0;
      }

    ASTERIA_PURE
    int
    compare(const IPv6_Address& other) const noexcept;

    // Returns the address class, which is shared by both IPv4 and IPv6.
    ASTERIA_PURE
    IP_Address_Class
    classify() const noexcept;

    // Checks whether this is an IPv4-mapped address.
    ASTERIA_PURE
    bool
    is_v4mapped() const noexcept
      {
        __m128i tval = _mm_load_si128(&(this->m_addr_stor));
        __m128i oval = _mm_setr_epi16(0, 0, 0, 0, 0, -1, 0, 0);
        int mask_ne = _mm_movemask_epi8(_mm_cmpeq_epi16(tval, oval)) ^ 0xFFFF;
        return (mask_ne & 0x0FFF) == 0;
      }

    // Parses an address from a string, which may be an IPv4 address, or
    // an IPv6 address in brackets, followed by a port number. Examples
    // are `127.0.0.1:80` and `[::1]:1300`. If an address has been parsed,
    // the number of characters that have been consumed is returned. If
    // zero is returned or an exception is thrown, the contents of this
    // object are unspecified.
    size_t
    parse(chars_view str) noexcept;

    // Converts this address to its string form. The caller should supply
    // a buffer for 48 characters, which is capable of storing the longest
    // string `[fedc:ba98:7654:3210:fedc:ba98:7654:3210]:65535`.
    size_t
    print_partial(char* str) const noexcept;

    tinyfmt&
    print_to(tinyfmt& fmt) const;

    cow_string
    to_string() const;
  };

struct IPv6_Address::hash
  {
    using result_type    = size_t;
    using argument_type  = IPv6_Address;

    result_type
    operator()(const IPv6_Address& addr) const noexcept
      {
        __m128i tval = _mm_load_si128(&(addr.m_addr_stor));
        tval = _mm_hadd_epi32(tval, _mm_bsrli_si128(tval, 8));
        tval = _mm_hadd_epi32(tval, tval);
        return static_cast<size_t>(_mm_cvtsi128_si64(tval)) + addr.m_port;
      }
  };

#define POSEIDON_IPV6(w0,w1,w2,w3,w4,w5,w6,w7, port)  \
    (::poseidon::IPv6_Address(::poseidon::IPv6_Address::fields{  \
          0x##w0, 0x##w1, 0x##w2, 0x##w3, 0x##w4, 0x##w5,  \
          0x##w6 >> 8, 0x##w6 & 0xFF, 0x##w7 >> 8, 0x##w7 & 0xFF, (port) }))

#define POSEIDON_IPV4(a,b,c,d, port)  \
    (::poseidon::IPv6_Address(::poseidon::IPv6_Address::fields{  \
          0, 0, 0, 0, 0, 0xFFFF, (a), (b), (c), (d), (port) }))

extern const IPv6_Address ipv6_unspecified;  // [::]:0
extern const IPv6_Address ipv6_loopback;     // [::1]:0
extern const IPv6_Address ipv6_invalid;      // a documentation-only address

extern const IPv6_Address ipv4_unspecified;  // [::ffff:0.0.0.0]:0
extern const IPv6_Address ipv4_loopback;     // [::ffff:127.0.0.1]:0
extern const IPv6_Address ipv4_broadcast;    // [::ffff:255.255.255.255]:0

inline
void
swap(IPv6_Address& lhs, IPv6_Address& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const IPv6_Address& addr)
  { return addr.print_to(fmt);  }

inline
bool
operator==(const IPv6_Address& lhs, const IPv6_Address& rhs) noexcept
  { return lhs.equals(rhs);  }

inline
bool
operator!=(const IPv6_Address& lhs, const IPv6_Address& rhs) noexcept
  { return !lhs.equals(rhs);  }

inline
bool
operator<(const IPv6_Address& lhs, const IPv6_Address& rhs) noexcept
  { return lhs.compare(rhs) < 0;  }

inline
bool
operator>(const IPv6_Address& lhs, const IPv6_Address& rhs) noexcept
  { return lhs.compare(rhs) > 0;  }

inline
bool
operator<=(const IPv6_Address& lhs, const IPv6_Address& rhs) noexcept
  { return lhs.compare(rhs) <= 0;  }

inline
bool
operator>=(const IPv6_Address& lhs, const IPv6_Address& rhs) noexcept
  { return lhs.compare(rhs) >= 0;  }

}  // namespace poseidon
#endif
