// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "udp_socket.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netinet/udp.h>
#include <net/if.h>
namespace poseidon {

UDP_Socket::
UDP_Socket(const IPv6_Address& addr)
  :
    Abstract_Socket(SOCK_DGRAM, IPPROTO_UDP)
  {
    static constexpr int true_value = 1;
    ::setsockopt(this->do_get_fd(), SOL_SOCKET, SO_REUSEADDR, &true_value, sizeof(int));

    ::sockaddr_in6 sa = { };
    sa.sin6_family = AF_INET6;
    sa.sin6_port = ROCKET_HTOBE16(addr.port());
    sa.sin6_addr = addr.addr();
    if(::bind(this->do_get_fd(), (const ::sockaddr*) &sa, sizeof(sa)) != 0)
      POSEIDON_THROW((
          "Failed to bind UDP socket onto `$1`",
          "[`bind()` failed: ${errno:full}]"),
          addr);

    POSEIDON_LOG_INFO(("UDP socket listening on `$1`"), this->local_address());
  }

UDP_Socket::
UDP_Socket()
  :
    Abstract_Socket(SOCK_DGRAM, IPPROTO_UDP)
  {
  }

UDP_Socket::
~UDP_Socket()
  {
  }

void
UDP_Socket::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_INFO((
        "UDP socket on `$3` closed: ${errno:full}",
        "[UDP socket `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address());
  }

void
UDP_Socket::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_read_queue(io_lock);
    ::ssize_t io_result = 0;

    for(;;) {
      // Try getting a packet.
      queue.clear();
      queue.reserve_after_end(0xFFFFU);
      ::sockaddr_in6 sa;
      ::socklen_t salen = sizeof(sa);
      io_result = ::recvfrom(this->do_get_fd(), queue.mut_end(), queue.capacity_after_end(), 0, (::sockaddr*) &sa, &salen);

      if(io_result < 0) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
          break;

        POSEIDON_LOG_DEBUG((
            "Error reading UDP socket",
            "[`recvfrom()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this));

        // Errors are ignored.
        continue;
      }

      if((sa.sin6_family != AF_INET6) || (salen != sizeof(sa)))
        continue;

      try {
        // Accept this incoming packet.
        this->m_taddr.set_addr(sa.sin6_addr);
        this->m_taddr.set_port(ROCKET_BETOH16(sa.sin6_port));
        queue.accept((size_t) io_result);

        this->do_on_udp_packet(move(this->m_taddr), move(queue));
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception thrown from `do_on_udp_packet()`: $3",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), stdex);
      }
    }

    POSEIDON_LOG_TRACE((
        "UDP socket `$1` (class `$2`): `do_abstract_socket_on_readable()` done"),
        this, typeid(*this));
  }

void
UDP_Socket::
do_abstract_socket_on_oob_readable()
  {
  }

void
UDP_Socket::
do_abstract_socket_on_writable()
  {
    this->do_abstract_socket_change_state(socket_pending, socket_established);
  }

void
UDP_Socket::
join_multicast_group(const IPv6_Address& maddr, uint8_t ttl, bool loopback, const char* ifname_opt)
  {
    // If no interface name is given, the second one is used, as the first one
    // is typically the loopback interface `lo`.
    unsigned ifindex = 2;
    if(ifname_opt) {
      // Get its index.
      ifindex = ::if_nametoindex(ifname_opt);
      if(ifindex <= 0)
        POSEIDON_THROW((
            "Failed to get index of interface `$3`",
            "[`ioctl()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), ifname_opt);
    }

    // IPv6 doesn't take IPv4-mapped multicast addresses, so there has to be
    // special treatement. `sendto()` is not affected.
    if(::memcmp(maddr.data(), ipv4_unspecified.data(), 12) == 0) {
      // Join the multicast group.
      ::ip_mreqn mreq;
      ::memcpy(&(mreq.imr_multiaddr.s_addr), maddr.data() + 12, 4);
      mreq.imr_address.s_addr = INADDR_ANY;
      mreq.imr_ifindex = (int) ifindex;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
        POSEIDON_THROW((
            "Failed to join IPv4 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);

      // Set multicast parameters.
      int value = ttl;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IP, IP_MULTICAST_TTL, &value, sizeof(int)) != 0)
        POSEIDON_THROW((
            "Failed to set TTL of IPv4 multicast packets",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this));

      value = loopback;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IP, IP_MULTICAST_LOOP, &value, sizeof(int)) != 0)
        POSEIDON_THROW((
            "Failed to set loopback of IPv4 multicast packets",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this));
    }
    else {
      // Join the multicast group.
      ::ipv6_mreq mreq;
      mreq.ipv6mr_multiaddr = maddr.addr();
      mreq.ipv6mr_interface = ifindex;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
        POSEIDON_THROW((
            "Failed to join IPv6 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);

      // Set multicast parameters.
      int value = ttl;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &value, sizeof(int)) != 0)
        POSEIDON_THROW((
            "Failed to set TTL of IPv6 multicast packets",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this));

      value = loopback;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &value, sizeof(int)) != 0)
        POSEIDON_THROW((
            "Failed to set loopback of IPv6 multicast packets",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this));
    }

    POSEIDON_LOG_INFO((
        "UDP socket has joined multicast group: address = `$3`, interface = `$4`",
        "[UDP socket `$1` (class `$2`)]"),
        this, typeid(*this), maddr, ifname_opt);
  }

void
UDP_Socket::
leave_multicast_group(const IPv6_Address& maddr, const char* ifname_opt)
  {
    // If no interface name is given, the second one is used, as the first one
    // is typically the loopback interface `lo`.
    unsigned ifindex = 2;
    if(ifname_opt) {
      // Get its index.
      ifindex = ::if_nametoindex(ifname_opt);
      if(ifindex <= 0)
        POSEIDON_THROW((
            "Failed to get index of interface `$3`",
            "[`ioctl()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), ifname_opt);
    }

    // IPv6 doesn't take IPv4-mapped multicast addresses, so there has to be
    // special treatement. `sendto()` is not affected.
    if(::memcmp(maddr.data(), ipv4_unspecified.data(), 12) == 0) {
      // Leave the multicast group.
      ::ip_mreqn mreq;
      ::memcpy(&(mreq.imr_multiaddr.s_addr), maddr.data() + 12, 4);
      mreq.imr_address.s_addr = INADDR_ANY;
      mreq.imr_ifindex = (int) ifindex;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
        POSEIDON_THROW((
            "Failed to leave IPv4 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);
    }
    else {
      // Leave the multicast group.
      ::ipv6_mreq mreq;
      mreq.ipv6mr_multiaddr = maddr.addr();
      mreq.ipv6mr_interface = ifindex;
      if(::setsockopt(this->do_get_fd(), IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) != 0)
        POSEIDON_THROW((
            "Failed to leave IPv6 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);
    }

    POSEIDON_LOG_INFO((
        "UDP socket has left multicast group: address = `$3`, interface = `$4`",
        "[UDP socket `$1` (class `$2`)]"),
        this, typeid(*this), maddr, ifname_opt);
  }

bool
UDP_Socket::
udp_send(const IPv6_Address& addr, chars_view data)
  {
    if((data.p == nullptr) && (data.n != 0))
      POSEIDON_THROW((
          "Null data pointer",
          "[UDP socket `$1` (class `$2`)]"),
          this, typeid(*this));

    if(data.n > UINT16_MAX)
      POSEIDON_THROW((
          "`$3` bytes is too large for a UDP packet",
          "[UDP socket `$1` (class `$2`)]"),
          this, typeid(*this), data.n);

    // If this socket has been marked closed, fail immediately.
    if(this->socket_state() == socket_closed)
      return false;

    // Try sending the packet immediately.
    // This is valid because UDP packets can be transmitted out of order.
    ::sockaddr_in6 sa;
    sa.sin6_family = AF_INET6;
    sa.sin6_port = ROCKET_HTOBE16(addr.port());
    sa.sin6_flowinfo = 0;
    sa.sin6_addr = addr.addr();
    sa.sin6_scope_id = 0;
    ::ssize_t io_result = ::sendto(this->do_get_fd(), data.p, data.n, 0, (const ::sockaddr*) &sa, sizeof(sa));

    if(io_result < 0) {
      POSEIDON_LOG_DEBUG((
          "Error writing UDP socket",
          "[`sendto()` failed: ${errno:full}]",
          "[UDP socket `$1` (class `$2`)]"),
          this, typeid(*this));

      // Errors are ignored.
      return false;
    }

    return true;
  }

}  // namespace poseidon
