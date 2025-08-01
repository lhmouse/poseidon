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
    static constexpr int one = 1;
    ::setsockopt(this->do_socket_fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    ::sockaddr_in6 sa = { };
    sa.sin6_family = AF_INET6;
    sa.sin6_port = ::htons(addr.port());
    sa.sin6_addr = addr.addr();
    if(::bind(this->do_socket_fd(), reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) != 0)
      POSEIDON_THROW((
          "Could not bind UDP socket to address `$1`",
          "[`bind()` failed: ${errno:full}]"),
          addr);

    POSEIDON_LOG_INFO((
        "UDP socket started listening on `$3`",
        "[UDP socket `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address());
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
        "UDP socket closed on `$3`: ${errno:full}",
        "[UDP socket `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address());
  }

void
UDP_Socket::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_read_queue(io_lock);
    auto& from_addr = this->m_from_addr;

    for(;;) {
      queue.clear();
      queue.reserve_after_end(0xFFFF);
      ::sockaddr_in6 sa;
      ::socklen_t salen = sizeof(sa);
      ::ssize_t ior = ::recvfrom(this->do_socket_fd(), queue.mut_end(),
                                 queue.capacity_after_end(), 0,
                                 reinterpret_cast<::sockaddr*>(&sa), &salen);
      if(ior < 0) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
          return;

        POSEIDON_LOG_DEBUG((
            "UDP socket read error: ${errno:full}",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this));

        // For UDP sockets, errors are ignored.
        continue;
      }

      queue.accept(static_cast<size_t>(ior));
      from_addr.set_addr(sa.sin6_addr);
      from_addr.set_port(::ntohs(sa.sin6_port));

      try {
        // Call the user-defined data callback.
        this->do_on_udp_packet(move(from_addr), move(queue));
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception: $3",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), stdex);

        // For UDP sockets, errors are ignored.
        continue;
      }

      POSEIDON_LOG_TRACE(("UDP socket `$1` (class `$2`) IN"), this, typeid(*this));
    }
  }

void
UDP_Socket::
do_abstract_socket_on_writeable()
  {
    this->do_socket_test_change(socket_pending, socket_established);
  }

void
UDP_Socket::
join_multicast_group(const IPv6_Address& maddr, const cow_string& ifname,
                     uint8_t ttl, bool loopback)
  {
    unsigned ifindex = ::if_nametoindex(ifname.safe_c_str());
    if(ifindex == 0)
      POSEIDON_THROW((
          "Unknown network interface interface `$3`",
          "[`if_nametoindex()` failed: ${errno:full}]",
          "[UDP socket `$1` (class `$2`)]"),
          this, typeid(*this), ifname);

    if(::memcmp(maddr.data(), ipv4_unspecified.data(), 12) == 0) {
      // IPv4
      ::ip_mreqn mr;
      ::memcpy(&(mr.imr_multiaddr.s_addr), maddr.data() + 12, 4);
      mr.imr_address.s_addr = INADDR_ANY;
      mr.imr_ifindex = static_cast<int>(ifindex);
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)) != 0)
        POSEIDON_THROW((
            "Could not join IPv4 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);

      int iv = ttl;
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IP, IP_MULTICAST_TTL, &iv, sizeof(iv)) != 0)
        POSEIDON_THROW((
            "Could not set IPv4 multicast TTL to `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), static_cast<int>(ttl));

      iv = loopback;
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IP, IP_MULTICAST_LOOP, &iv, sizeof(iv)) != 0)
        POSEIDON_THROW((
            "Could not set IPv4 multicast loopback to `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), static_cast<int>(loopback));
    }
    else {
      // IPv6
      ::ipv6_mreq mr;
      mr.ipv6mr_multiaddr = maddr.addr();
      mr.ipv6mr_interface = ifindex;
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mr, sizeof(mr)) != 0)
        POSEIDON_THROW((
            "Could not join IPv6 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);

      int iv = ttl;
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &iv, sizeof(iv)) != 0)
        POSEIDON_THROW((
            "Could not set IPv6 multicast TTL to `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), static_cast<int>(ttl));

      iv = loopback;
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &iv, sizeof(iv)) != 0)
        POSEIDON_THROW((
            "Could not set IPv6 multicast loopback to `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), static_cast<int>(loopback));
    }

    POSEIDON_LOG_INFO((
        "UDP socket joined multicast group `$3` on interface `$4`",
        "[UDP socket `$1` (class `$2`)]"),
        this, typeid(*this), maddr, ifname);
  }

void
UDP_Socket::
leave_multicast_group(const IPv6_Address& maddr, const cow_string& ifname)
  {
    unsigned ifindex = ::if_nametoindex(ifname.safe_c_str());
    if(ifindex == 0)
      POSEIDON_THROW((
          "Unknown network interface interface `$3`",
          "[`if_nametoindex()` failed: ${errno:full}]",
          "[UDP socket `$1` (class `$2`)]"),
          this, typeid(*this), ifname);

    if(::memcmp(maddr.data(), ipv4_unspecified.data(), 12) == 0) {
      // IPv4
      ::ip_mreqn mr;
      ::memcpy(&(mr.imr_multiaddr.s_addr), maddr.data() + 12, 4);
      mr.imr_address.s_addr = INADDR_ANY;
      mr.imr_ifindex = static_cast<int>(ifindex);
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IP, IP_DROP_MEMBERSHIP, &mr, sizeof(mr)) != 0)
        POSEIDON_THROW((
            "Could not leave IPv4 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);
    }
    else {
      // IPv6
      ::ipv6_mreq mr;
      mr.ipv6mr_multiaddr = maddr.addr();
      mr.ipv6mr_interface = ifindex;
      if(::setsockopt(this->do_socket_fd(), IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mr, sizeof(mr)) != 0)
        POSEIDON_THROW((
            "Could not leave IPv6 multicast group `$3`",
            "[`setsockopt()` failed: ${errno:full}]",
            "[UDP socket `$1` (class `$2`)]"),
            this, typeid(*this), maddr);
    }

    POSEIDON_LOG_INFO((
        "UDP socket left multicast group `$3` on interface `$4`",
        "[UDP socket `$1` (class `$2`)]"),
        this, typeid(*this), maddr, ifname);
  }

bool
UDP_Socket::
udp_send(const IPv6_Address& addr, chars_view data)
  {
    if(this->socket_state() >= socket_closing)
      return false;

    ::sockaddr_in6 sa = { };
    sa.sin6_family = AF_INET6;
    sa.sin6_port = ::htons(addr.port());
    sa.sin6_addr = addr.addr();
    ::ssize_t ior = ::sendto(this->do_socket_fd(), data.p, data.n, 0,
                             reinterpret_cast<::sockaddr*>(&sa), sizeof(sa));
    if(ior < 0) {
      POSEIDON_LOG_DEBUG((
          "UDP socket write error: ${errno:full}",
          "[UDP socket `$1` (class `$2`)]"),
          this, typeid(*this));

      // Errors are ignored.
      return false;
    }

    return true;
  }

}  // namespace poseidon
