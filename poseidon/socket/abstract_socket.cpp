// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "abstract_socket.hpp"
#include "enums.hpp"
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
      POSEIDON_THROW(("Null socket handle not valid"));

    // Check the address family.
    if(this->local_address() == ipv6_invalid)
      POSEIDON_THROW(("Could not get socket local address"));

#ifdef ROCKET_DEBUG
    // Require the socket be non-blocking here.
    int fl = ::fcntl(this->m_fd, F_GETFL);
    ROCKET_ASSERT(fl != -1);
    ROCKET_ASSERT(fl & O_NONBLOCK);
#endif
  }

Abstract_Socket::
Abstract_Socket(int type, int protocol)
  {
    // Create a non-blocking socket.
    this->m_fd.reset(::socket(AF_INET6, type | SOCK_NONBLOCK, protocol));
    if(!this->m_fd)
      POSEIDON_THROW((
          "Could not create IPv6 socket: type `$1`, protocol `$2`",
          "[`socket()` failed: ${errno:full}]"),
          type, protocol);

    // Use `TCP_NODELAY`. Errors are ignored.
    static constexpr int true_value = 1;
    if(type == SOCK_STREAM)
      ::setsockopt(this->m_fd, IPPROTO_TCP, TCP_NODELAY, &true_value, sizeof(int));
  }

Abstract_Socket::
~Abstract_Socket()
  {
  }

const Socket_Address&
Abstract_Socket::
local_address() const noexcept
  {
    if(this->m_sockname_ready.load())
      return this->m_sockname;

    // Try getting the address now.
    static plain_mutex s_mutex;
    plain_mutex::unique_lock lock(s_mutex);

    if(this->m_sockname_ready.load())
      return this->m_sockname;

    ::sockaddr_in6 sa;
    ::socklen_t salen = sizeof(sa);
    if(::getsockname(this->m_fd, (::sockaddr*) &sa, &salen) != 0)
      return ipv6_invalid;

    ROCKET_ASSERT(sa.sin6_family == AF_INET6);
    ROCKET_ASSERT(salen == sizeof(sa));

    if(sa.sin6_port == ROCKET_HTOBE16(0))
      return ipv6_unspecified;

    // Cache the address.
    this->m_sockname.set_addr(sa.sin6_addr);
    this->m_sockname.set_port(ROCKET_BETOH16(sa.sin6_port));
    ::std::atomic_thread_fence(::std::memory_order_release);
    this->m_sockname_ready.store(true);
    return this->m_sockname;
  }

void
Abstract_Socket::
connect(const Socket_Address& addr)
  {
    ::sockaddr_in6 sa;
    sa.sin6_family = AF_INET6;
    sa.sin6_port = ROCKET_HTOBE16(addr.port());
    sa.sin6_flowinfo = 0;
    sa.sin6_addr = addr.addr();
    sa.sin6_scope_id = 0;
    if((::connect(this->m_fd, (const ::sockaddr*) &sa, sizeof(sa)) != 0) && (errno != EINPROGRESS))
      POSEIDON_THROW((
          "Failed to initiate TCP connection to `$3`",
          "[`connect()` failed: ${errno:full}]",
          "[TCP socket `$1` (class `$2`)]"),
          this, typeid(*this), addr);
  }

bool
Abstract_Socket::
quick_close() noexcept
  {
    if(this->m_state.load() == socket_closed)
      return true;

    // It doesn't matter if this gets called multiple times.
    this->m_state.store(socket_closed);

    // Discard pending data.
    ::linger lng;
    lng.l_onoff = 1;
    lng.l_linger = 0;
    ::setsockopt(this->m_fd, SOL_SOCKET, SO_LINGER, &lng, sizeof(lng));
    return ::shutdown(this->m_fd, SHUT_RDWR) == 0;
  }

}  // namespace poseidon
