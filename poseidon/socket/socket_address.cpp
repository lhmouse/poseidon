// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "socket_address.hpp"
#include "../utils.hpp"
#include <arpa/inet.h>
#include <http_parser.h>
namespace poseidon {
namespace {

ROCKET_ALWAYS_INLINE
bool
do_match_subnet(const char* addr, const char* mask, uint32_t bits) noexcept
  {
    uint32_t bi = bits / 8;
    return (::memcmp(addr, mask, bi) == 0)
           && (((addr[bi] ^ mask[bi]) & 0xFF & (0xFF00 >> bits % 8)) == 0);
  }

inline
IP_Address_Class
do_classify_ipv4_generic(const char* addr) noexcept
  {
    // 0.0.0.0/32: Unspecified
    if(do_match_subnet(addr, "\x00\x00\x00\x00", 32))
      return ip_address_class_unspecified;

    // 0.0.0.0/8: Local Identification
    if(do_match_subnet(addr, "\x00", 8))
      return ip_address_class_reserved;

    // 10.0.0.0/8: Class A Private-Use
    if(do_match_subnet(addr, "\x0A", 8))
      return ip_address_class_private;

    // 127.0.0.0/8: Loopback
    if(do_match_subnet(addr, "\x7F", 8))
      return ip_address_class_loopback;

    // 172.16.0.0/12: Class B Private-Use
    if(do_match_subnet(addr, "\xAC\x10", 12))
      return ip_address_class_private;

    // 169.254.0.0/16: Link Local
    if(do_match_subnet(addr, "\xA9\xFE", 16))
      return ip_address_class_link_local;

    // 192.168.0.0/16: Class C Private-Use
    if(do_match_subnet(addr, "\xC0\xA8", 16))
      return ip_address_class_private;

    // 224.0.0.0/4: Class D Multicast
    if(do_match_subnet(addr, "\xE0", 4))
      return ip_address_class_multicast;

    // 240.0.0.0/4: Class E
    if(do_match_subnet(addr, "\xF0", 4))
      return ip_address_class_reserved;

    // 255.255.255.255/32: Broadcast
    if(do_match_subnet(addr, "\xFF\xFF\xFF\xFF", 32))
      return ip_address_class_broadcast;

    // Default
    return ip_address_class_public;
  }

inline
IP_Address_Class
do_classify_ipv6_generic(const char* addr) noexcept
  {
    // ::ffff:0:0/96: IPv4-mapped
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 96))
      return do_classify_ipv4_generic(addr + 12);

    // ::/128: Unspecified
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 128))
      return ip_address_class_unspecified;

    // ::1/128: Loopback
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 128))
      return ip_address_class_loopback;

    // 64:ff9b::/96: IPv4 to IPv6
    if(do_match_subnet(addr, "\x00\x64\xFF\x9B\x00\x00\x00\x00\x00\x00\x00\x00", 96))
      return do_classify_ipv4_generic(addr + 12);

    // 64:ff9b:1::/48: Local-Use IPv4/IPv6
    if(do_match_subnet(addr, "\x00\x64\xFF\x9B\x00\x01", 48))
      return do_classify_ipv4_generic(addr + 12);

    // 100::/64: Discard-Only
    if(do_match_subnet(addr, "\x01\x00\x00\x00\x00\x00\x00\x00", 64))
      return ip_address_class_reserved;

    // 2001:db8::/32: Documentation
    if(do_match_subnet(addr, "\x20\x01\x0D\xB8", 32))
      return ip_address_class_reserved;

    // 2002::/16: 6to4
    if(do_match_subnet(addr, "\x20\x02", 16))
      return do_classify_ipv4_generic(addr + 2);

    // fc00::/7: Unique Local Unicast
    if(do_match_subnet(addr, "\xFC\x00", 7))
      return ip_address_class_private;

    // fe80::/10: Link-Scoped Unicast
    if(do_match_subnet(addr, "\xFE\x80", 10))
      return ip_address_class_link_local;

    // ff00::/8: Multicast
    if(do_match_subnet(addr, "\xFF\x00", 8))
      return ip_address_class_multicast;

    // Default
    return ip_address_class_public;
  }

}  // namespace

const Socket_Address ipv6_unspecified  = (::in6_addr) { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
const Socket_Address ipv6_loopback     = (::in6_addr) { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 };
const Socket_Address ipv6_invalid      = (::in6_addr) { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

const Socket_Address ipv4_unspecified  = (::in6_addr) { 0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,0 };
const Socket_Address ipv4_loopback     = (::in6_addr) { 0,0,0,0,0,0,0,0,0,0,255,255,127,0,0,1 };
const Socket_Address ipv4_broadcast    = (::in6_addr) { 0,0,0,0,0,0,0,0,0,0,255,255,255,255,255,255 };

Socket_Address::
Socket_Address(const char* str, size_t len)
  {
    size_t r = this->parse(str, len);
    if(r != len)
      POSEIDON_THROW((
          "Could not parse socket address string `$1`"),
          cow_string(str, len));
  }

Socket_Address::
Socket_Address(const char* str)
  {
    size_t r = this->parse(str, ::strlen(str));
    if(str[r] != 0)
      POSEIDON_THROW((
          "Could not parse socket address string `$1`"),
          str);
  }

Socket_Address::
Socket_Address(cow_stringR str)
  {
    size_t r = this->parse(str.data(), str.size());
    if(r != str.size())
      POSEIDON_THROW((
          "Could not parse socket address string `$1`"),
          str);
  }

int
Socket_Address::
compare(const Socket_Address& other) const noexcept
  {
    char tdata[18];
    ::memcpy(tdata, &(this->m_addr), 16);
    uint16_t port_be = htobe16(this->m_port);
    ::memcpy(tdata + 16, &port_be, 2);

    char odata[18];
    ::memcpy(odata, &(other.m_addr), 16);
    port_be = htobe16(other.m_port);
    ::memcpy(odata + 16, &port_be, 2);

    return ::memcmp(tdata, odata, 18);
  }

IP_Address_Class
Socket_Address::
classify() const noexcept
  {
    return do_classify_ipv6_generic((const char*) &(this->m_addr));
  }

size_t
Socket_Address::
parse(const char* str, size_t len) noexcept
  {
    this->clear();

    if((len == 0) || (len > UINT16_MAX))
      return 0;

    // Break down the host:port string as a URL.
    ::http_parser_url url;
    url.field_set = 0;
    if(::http_parser_parse_url(str, len, true, &url) != 0)
      return 0;

    char* addr = (char*) &(this->m_addr);
    char sbuf[64];
    const char* host = str + url.field_data[UF_HOST].off;
    size_t hostlen = url.field_data[UF_HOST].len;

    if((hostlen < 1) || (hostlen > 63))
      return 0;

    ::memcpy(sbuf, host, hostlen);
    sbuf[hostlen] = 0;

    if(host[hostlen] != ']') {
      // IPv4
      ::memcpy(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 12);
      addr += 12;
      if(::inet_pton(AF_INET, sbuf, addr) == 0)
        return 0;
    }
    else {
      // IPv6
      if(::inet_pton(AF_INET6, sbuf, addr) == 0)
        return 0;
    }
    this->m_port = url.port;

    // Return the number of characters up to the end of the port field.
    return (size_t) url.field_data[UF_PORT].off + url.field_data[UF_PORT].len;
  }

size_t
Socket_Address::
print_partial(char* str) const noexcept
  {
    const char* addr = (const char*) &(this->m_addr);
    ::rocket::ascii_numput nump;
    char* wptr = str;

    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 96)) {
      // IPv4
      addr += 12;
      if(::inet_ntop(AF_INET, addr, wptr, INET_ADDRSTRLEN) == nullptr)
        return (size_t) (::stpcpy(wptr, "(invalid IPv4 address)") - str);

      wptr += ::strlen(wptr);
    }
    else {
      // IPv6
      xstrrpcpy(wptr, "[");
      if(::inet_ntop(AF_INET6, addr, wptr, INET6_ADDRSTRLEN) == nullptr)
        return (size_t) (::stpcpy(wptr, "(invalid IPv6 address)") - str);

      wptr += ::strlen(wptr);
      xstrrpcpy(wptr, "]");
    }
    xstrrpcpy(wptr, ":");
    nump.put_DU(this->m_port);
    xmemrpcpy(wptr, nump.data(), nump.size());

    // Return the number of characters that have been written in total.
    return (size_t) (wptr - str);
  }

tinyfmt&
Socket_Address::
print(tinyfmt& fmt) const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return fmt.putn(str, len);
  }

cow_string
Socket_Address::
print_to_string() const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return cow_string(str, len);
  }

}  // namespace poseidon
