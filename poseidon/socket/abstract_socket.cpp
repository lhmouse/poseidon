// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "abstract_socket.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>
namespace poseidon {

Abstract_Socket::
Abstract_Socket(unique_posix_fd&& fd)
  {
    // Take ownership the socket handle.
    this->m_fd = move(fd);
    if(!this->m_fd)
      POSEIDON_THROW(("Null socket descriptor"));

    // Require the socket be non-blocking here for simplicity.
    int fl_old = ::fcntl(this->m_fd, F_GETFL);
    if(fl_old == -1)
      POSEIDON_THROW(("Could not get socket flags: ${errno:full}]"));

    int fl_new = fl_old | O_NONBLOCK;
    if((fl_new != fl_old) && (::fcntl(this->m_fd, F_SETFL, fl_new) != 0))
      POSEIDON_THROW(("Could not set socket flags: ${errno:full}]"));

    this->m_scheduler = reinterpret_cast<Network_Scheduler*>(-1);
  }

Abstract_Socket::
Abstract_Socket(int type, int protocol)
  {
    // Create a non-blocking socket.
    this->m_fd.reset(::socket(AF_INET6, type | SOCK_NONBLOCK, protocol));
    if(!this->m_fd)
      POSEIDON_THROW((
          "Could not create IPv6 socket of type `$1` and protocol `$2`",
          "[`socket()` failed: ${errno:full}]"),
          type, protocol);

    // Use `TCP_QUICKACK`. Errors are ignored.
    static constexpr int one = 1;
    if((type == SOCK_STREAM) && is_any_of(protocol, { IPPROTO_IP, IPPROTO_TCP }))
      ::setsockopt(this->m_fd, IPPROTO_TCP, TCP_QUICKACK, &one, sizeof(one));

    this->m_scheduler = reinterpret_cast<Network_Scheduler*>(-3);
  }

Abstract_Socket::
~Abstract_Socket()
  {
  }

Network_Scheduler&
Abstract_Socket::
do_abstract_socket_lock_scheduler(recursive_mutex::unique_lock& lock)
  const noexcept
  {
    lock.lock(this->m_sched_mutex);
    ROCKET_ASSERT(this->m_scheduler);
    return *(this->m_scheduler);
  }

linear_buffer&
Abstract_Socket::
do_abstract_socket_lock_read_queue(recursive_mutex::unique_lock& lock)
  noexcept
  {
    lock.lock(this->m_sched_mutex);
    return this->m_sched_read_queue;
  }

linear_buffer&
Abstract_Socket::
do_abstract_socket_lock_write_queue(recursive_mutex::unique_lock& lock)
  noexcept
  {
    lock.lock(this->m_sched_mutex);
    return this->m_sched_write_queue;
  }

const IPv6_Address&
Abstract_Socket::
local_address()
  const noexcept
  {
    if(this->m_sockname_ready.load())
      return this->m_sockname;

    // Try getting the local address now.
    int old_err = errno;
    ::sockaddr_in6 sa;
    ::socklen_t salen = sizeof(sa);
    if(::getsockname(this->m_fd, reinterpret_cast<::sockaddr*>(&sa), &salen) != 0) {
      errno = old_err;
      return ipv6_invalid;
    }

    if((sa.sin6_family != AF_INET6) || (salen < sizeof(sa)))
      return ipv6_invalid;

    // If this is an unspecified address, don't cache it.
    if(sa.sin6_port == 0)
      return ipv6_unspecified;

    // Save the result.
    this->m_sockname.set_addr(sa.sin6_addr);
    this->m_sockname.set_port(ROCKET_BETOH16(sa.sin6_port));
    this->m_sockname_ready.store(true);  // release
    return this->m_sockname;
  }

const IPv6_Address&
Abstract_Socket::
remote_address()
  const noexcept
  {
    if(this->m_peername_ready.load())
      return this->m_peername;

    // Try getting the remote address now.
    int old_err = errno;
    ::sockaddr_in6 sa;
    ::socklen_t salen = sizeof(sa);
    if(::getpeername(this->m_fd, reinterpret_cast<::sockaddr*>(&sa), &salen) != 0) {
      ::std::swap(errno, old_err);
      return (old_err == ENOTCONN) ? ipv6_unspecified : ipv6_invalid;
    }

    if((sa.sin6_family != AF_INET6) || (salen < sizeof(sa)))
      return ipv6_invalid;

    // If this is an unspecified address, don't cache it.
    if(sa.sin6_port == 0)
      return ipv6_unspecified;

    // Save the result.
    this->m_peername.set_addr(sa.sin6_addr);
    this->m_peername.set_port(ROCKET_BETOH16(sa.sin6_port));
    this->m_peername_ready.store(true);  // release
    return this->m_peername;
  }

bool
Abstract_Socket::
quick_shut_down()
  noexcept
  {
    int r = ::shutdown(this->m_fd, SHUT_RDWR);
    this->m_state.store(socket_closed);
    return r == 0;
  }

}  // namespace poseidon
