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
array<uint8_t, 18>
do_copy_bytes_be(const ::in6_addr& addr, uint16_t port) noexcept
  {
    array<uint8_t, 18> bytes;
    void* wptr = bytes.begin();
    nmempcpy(wptr, &addr, sizeof(addr));
    uint16_t beport = htobe16(port);
    nmempcpy(wptr, &beport, sizeof(beport));
    ROCKET_ASSERT(wptr == bytes.end());
    return bytes;
  }

ROCKET_ALWAYS_INLINE
bool
do_match_subnet(const void* addr, const void* mask, uint32_t bits) noexcept
  {
    return (::memcmp(addr, mask, bits / 8) == 0) &&
           (((*((const uint8_t*) addr + bits / 8) ^ *((const uint8_t*) mask + bits / 8))
             & (0xFF00U >> bits % 8)) == 0);
  }

inline
IP_Address_Class
do_classify_ipv4_generic(const void* addr) noexcept
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
do_classify_ipv6_generic(const void* addr) noexcept
  {
    // ::ffff:0:0/96: IPv4-mapped
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 96))
      return do_classify_ipv4_generic((const uint8_t*) addr + 12);

    // ::/128: Unspecified
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 128))
      return ip_address_class_unspecified;

    // ::1/128: Loopback
    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 128))
      return ip_address_class_loopback;

    // 64:ff9b::/96: IPv4 to IPv6
    if(do_match_subnet(addr, "\x00\x64\xFF\x9B\x00\x00\x00\x00\x00\x00\x00\x00", 96))
      return do_classify_ipv4_generic((const uint8_t*) addr + 12);

    // 64:ff9b:1::/48: Local-Use IPv4/IPv6
    if(do_match_subnet(addr, "\x00\x64\xFF\x9B\x00\x01", 48))
      return do_classify_ipv4_generic((const uint8_t*) addr + 12);

    // 100::/64: Discard-Only
    if(do_match_subnet(addr, "\x01\x00\x00\x00\x00\x00\x00\x00", 64))
      return ip_address_class_reserved;

    // 2001:db8::/32: Documentation
    if(do_match_subnet(addr, "\x20\x01\x0D\xB8", 32))
      return ip_address_class_reserved;

    // 2002::/16: 6to4
    if(do_match_subnet(addr, "\x20\x02", 16))
      return do_classify_ipv4_generic((const uint8_t*) addr + 2);

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

const Socket_Address ipv6_unspecified  = (::in6_addr) {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const Socket_Address ipv6_loopback     = (::in6_addr) {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const Socket_Address ipv6_invalid      = (::in6_addr) {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const Socket_Address ipv4_unspecified  = (::in6_addr) {0,0,0,0,0,0,0,0,0,0,255,255,0,0,0,0};
const Socket_Address ipv4_loopback     = (::in6_addr) {0,0,0,0,0,0,0,0,0,0,255,255,127,0,0,1};
const Socket_Address ipv4_broadcast    = (::in6_addr) {0,0,0,0,0,0,0,0,0,0,255,255,255,255,255,255};

Socket_Address::
Socket_Address(const char* str, size_t len)
  {
    if(this->parse(str, len) == 0)
      POSEIDON_THROW((
          "Could not parse socket address string `$1`"),
          string(str, len));
  }

Socket_Address::
Socket_Address(const char* str)
  {
    if(this->parse(str) == 0)
      POSEIDON_THROW((
          "Could not parse socket address string `$1`"),
          str);
  }

Socket_Address::
Socket_Address(stringR str)
  {
    if(this->parse(str) == 0)
      POSEIDON_THROW((
          "Could not parse socket address string `$1`"),
          str);
  }

int
Socket_Address::
compare(const Socket_Address& other) const noexcept
  {
    auto bl = do_copy_bytes_be(this->m_addr, this->m_port);
    auto br = do_copy_bytes_be(other.m_addr, other.m_port);
    return ::memcmp(bl.data(), br.data(), br.size());
  }

IP_Address_Class
Socket_Address::
classify() const noexcept
  {
    return do_classify_ipv6_generic(&(this->m_addr));
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

    const char* host = str + url.field_data[UF_HOST].off;
    size_t hostlen = url.field_data[UF_HOST].len;
    uint8_t* addr = (uint8_t*) &(this->m_addr);
    char sbuf[64];

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
parse(const char* str) noexcept
  {
    return this->parse(str, ::strlen(str));
  }

size_t
Socket_Address::
parse(stringR str) noexcept
  {
    return this->parse(str.data(), str.size());
  }

size_t
Socket_Address::
print_partial(char* str) const noexcept
  {
    const uint8_t* addr = (const uint8_t*) &(this->m_addr);
    ::rocket::ascii_numput nump;
    char* wptr = str;

    if(do_match_subnet(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 96)) {
      // IPv4
      addr += 12;
      if(::inet_ntop(AF_INET, addr, wptr, INET_ADDRSTRLEN) == nullptr)
        return nstpcpy(wptr, "(invalid IPv4 address)");

      wptr += ::strlen(wptr);
    }
    else {
      // IPv6
      nstpset(wptr, '[');
      if(::inet_ntop(AF_INET6, addr, wptr, INET6_ADDRSTRLEN) == nullptr)
        return nstpcpy(wptr, "(invalid IPv6 address)");

      wptr += ::strlen(wptr);
      nstpset(wptr, ']');
    }
    nstpset(wptr, ':');
    nump.put_DU(this->m_port);
    nstpcpy(wptr, nump.data(), nump.size());

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

string
Socket_Address::
print_to_string() const
  {
    char str[64];
    size_t len = this->print_partial(str);
    return string(str, len);
  }

}  // namespace poseidon
