// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "ipv6_address.hpp"
#include "../utils.hpp"
#include <arpa/inet.h>
namespace poseidon {
namespace {

ROCKET_ALWAYS_INLINE
bool
do_match_subnet(const char* addr, const char* mask, uint32_t bits)
  noexcept
  {
    uint32_t bi = bits / 8;
    return (::memcmp(addr, mask, bi) == 0)
           && ((0xFF & (0xFF00 >> bits % 8) & (addr[bi] ^ mask[bi])) == 0);
  }

inline
IP_Address_Class
do_classify_ipv4_generic(const char* addr)
  noexcept
  {
    // 0.0.0.0/32: Unspecified
    if(do_match_subnet(addr, "\x00\x00\x00\x00", 32))
      return ip_address_unspecified;

    // 0.0.0.0/8: Local Identification
    if(do_match_subnet(addr, "\x00", 8))
      return ip_address_reserved;

    // 10.0.0.0/8: Class A Private-Use
    if(do_match_subnet(addr, "\x0A", 8))
      return ip_address_private;

    // 127.0.0.0/8: Loopback
    if(do_match_subnet(addr, "\x7F", 8))
      return ip_address_loopback;

    // 172.16.0.0/12: Class B Private-Use
    if(do_match_subnet(addr, "\xAC\x10", 12))
      return ip_address_private;

    // 169.254.0.0/16: Link Local
    if(do_match_subnet(addr, "\xA9\xFE", 16))
      return ip_address_link_local;

    // 192.168.0.0/16: Class C Private-Use
    if(do_match_subnet(addr, "\xC0\xA8", 16))
      return ip_address_private;

    // 224.0.0.0/4: Class D Multicast
    if(do_match_subnet(addr, "\xE0", 4))
      return ip_address_multicast;

    // 240.0.0.0/4: Class E
    if(do_match_subnet(addr, "\xF0", 4))
      return ip_address_reserved;

    // 255.255.255.255/32: Broadcast
    if(do_match_subnet(addr, "\xFF\xFF\xFF\xFF", 32))
      return ip_address_broadcast;

    // Default
    return ip_address_public;
  }

inline
IP_Address_Class
do_classify_ipv6_generic(const char* addr)
  noexcept
  {
    // ::ffff:0:0/96: IPv4-mapped
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 96))
      return do_classify_ipv4_generic(addr + 12);

    // ::/128: Unspecified
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 128))
      return ip_address_unspecified;

    // ::1/128: Loopback
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 128))
      return ip_address_loopback;

    // 64:ff9b::/96: IPv4 to IPv6
    if(do_match_subnet(addr, "\x00\x64\xFF\x9B\x00\x00\x00\x00\x00\x00\x00\x00", 96))
      return do_classify_ipv4_generic(addr + 12);

    // 64:ff9b:1::/48: Local-Use IPv4/IPv6
    if(do_match_subnet(addr, "\x00\x64\xFF\x9B\x00\x01", 48))
      return do_classify_ipv4_generic(addr + 12);

    // 100::/64: Discard-Only
    if(do_match_subnet(addr, "\x01\x00\x00\x00\x00\x00\x00\x00", 64))
      return ip_address_reserved;

    // 2001:db8::/32: Documentation
    if(do_match_subnet(addr, "\x20\x01\x0D\xB8", 32))
      return ip_address_reserved;

    // 2002::/16: 6to4
    if(do_match_subnet(addr, "\x20\x02", 16))
      return do_classify_ipv4_generic(addr + 2);

    // fc00::/7: Unique Local Unicast
    if(do_match_subnet(addr, "\xFC\x00", 7))
      return ip_address_private;

    // fe80::/10: Link-Scoped Unicast
    if(do_match_subnet(addr, "\xFE\x80", 10))
      return ip_address_link_local;

    // ff00::/8: Multicast
    if(do_match_subnet(addr, "\xFF\x00", 8))
      return ip_address_multicast;

    // Default
    return ip_address_public;
  }

}  // namespace

#define DOCUMENTATION_   32,1,13,184,243,151,214,23,74,162,130,224,   // 2001:db8::/32; random
#define IPV4_MAPPED_     0,0,0,0,0,0,0,0,0,0,255,255,  // ::ffff:0:0/96

const IPv6_Address ipv6_unspecified   = { (::in6_addr)   IN6ADDR_ANY_INIT,                0 };
const IPv6_Address ipv6_loopback      = { (::in6_addr)   IN6ADDR_LOOPBACK_INIT,           0 };
const IPv6_Address ipv6_invalid       = { (::in6_addr) { DOCUMENTATION_ 44,95,237,217 },  0 };

const IPv6_Address ipv4_unspecified   = { (::in6_addr) { IPV4_MAPPED_ 0,0,0,0 },          0 };
const IPv6_Address ipv4_loopback      = { (::in6_addr) { IPV4_MAPPED_ 127,0,0,1 },        0 };
const IPv6_Address ipv4_broadcast     = { (::in6_addr) { IPV4_MAPPED_ 255,255,255,255 },  0 };

IPv6_Address::
IPv6_Address(chars_view str)
  {
    if(this->parse(str) != str.n)
      POSEIDON_THROW(("Could not parse socket address string `$1`"), str);
  }

int
IPv6_Address::
compare(const IPv6_Address& other)
  const noexcept
  {
    char tdata[18];
    ::memcpy(tdata, &(this->m_addr), 16);
    ROCKET_STORE_BE16(tdata + 16, this->m_port);

    char odata[18];
    ::memcpy(odata, &(other.m_addr), 16);
    ROCKET_STORE_BE16(odata + 16, other.m_port);

    return ::memcmp(tdata, odata, 18);
  }

IP_Address_Class
IPv6_Address::
classify()
  const noexcept
  {
    return do_classify_ipv6_generic((const char*) &(this->m_addr));
  }

size_t
IPv6_Address::
parse(chars_view str)
  noexcept
  {
    Network_Reference caddr;
    size_t aclen = parse_network_reference(caddr, str);

    // The URI string shall only contain the host and port parts.
    if((caddr.host.n == 0) || (caddr.host.n > 63))
      return 0;

    if((caddr.port.n == 0) || (caddr.path.n != 0) || (caddr.query.n != 0)  || (caddr.fragment.n != 0))
      return 0;

    // Null-terminate the host name.
    char sbuf[64];
    ::memcpy(sbuf, caddr.host.p, caddr.host.n);
    sbuf[caddr.host.n] = 0;

    if(caddr.is_ipv6) {
      // IPv6
      if(::inet_pton(AF_INET6, sbuf, &(this->m_addr)) == 0)
        return 0;
    }
    else {
      // IPv4-mapped
      this->m_addr = ipv4_unspecified.m_addr;
      if(::inet_pton(AF_INET, sbuf, (char*) &(this->m_addr) + 12) == 0)
        return 0;
    }

    // port
    this->m_port = caddr.port_num;
    return aclen;
  }

size_t
IPv6_Address::
print_partial(char* str)
  const noexcept
  {
    const char* addr = (const char*) &(this->m_addr);
    ::rocket::ascii_numput nump;
    char* wptr = str;

    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 96)) {
      // IPv4
      addr += 12;
      ::inet_ntop(AF_INET, addr, wptr, INET_ADDRSTRLEN);
      wptr += ::strlen(wptr);
    }
    else {
      // IPv6
      xstrrpcpy(wptr, "[");
      ::inet_ntop(AF_INET6, addr, wptr, INET6_ADDRSTRLEN);
      wptr += ::strlen(wptr);
      xstrrpcpy(wptr, "]");
    }

    // port
    xstrrpcpy(wptr, ":");
    nump.put_DU(this->m_port);
    xmemrpcpy(wptr, nump.data(), nump.size());

    // Return the number of characters that have been written in total.
    return (size_t) (wptr - str);
  }

tinyfmt&
IPv6_Address::
print_to(tinyfmt& fmt)
  const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return fmt.putn(str, len);
  }

cow_string
IPv6_Address::
to_string()
  const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
