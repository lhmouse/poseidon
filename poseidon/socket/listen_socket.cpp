// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "listen_socket.hpp"
#include "../static/network_driver.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netinet/tcp.h>
namespace poseidon {

Listen_Socket::
Listen_Socket(const Socket_Address& addr)
  :
    Abstract_Socket(SOCK_STREAM, IPPROTO_TCP)
  {
    // Use `SO_REUSEADDR`. Errors are ignored.
    static constexpr int true_value = 1;
    ::setsockopt(this->do_get_fd(), SOL_SOCKET, SO_REUSEADDR, &true_value, sizeof(int));

    // Bind this socket onto `addr`.
    ::sockaddr_in6 sa;
    sa.sin6_family = AF_INET6;
    sa.sin6_port = htobe16(addr.port());
    sa.sin6_flowinfo = 0;
    sa.sin6_addr = addr.addr();
    sa.sin6_scope_id = 0;

    if(::bind(this->do_get_fd(), (const ::sockaddr*) &sa, sizeof(sa)) != 0)
      POSEIDON_THROW((
          "Failed to bind TCP socket onto `$3`",
          "[`bind()` failed: ${errno:full}]",
          "[TCP socket `$1` (class `$2`)]"),
          this, typeid(*this), addr);

    if(::listen(this->do_get_fd(), SOMAXCONN) != 0)
      POSEIDON_THROW((
          "Failed to start listening on `$3`",
          "[`listen()` failed: ${errno:full}]",
          "[TCP listen socket `$1` (class `$2`)]"),
          this, typeid(*this), addr);

    POSEIDON_LOG_INFO((
        "TCP server started listening on `$3`",
        "[TCP socket `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address());
  }

Listen_Socket::
~Listen_Socket()
  {
    POSEIDON_LOG_INFO(("Destroying `$1` (class `$2`)"), this, typeid(*this));
  }

void
Listen_Socket::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_INFO((
        "TCP server stopped listening on `$3`: ${errno:full}",
        "[TCP listen socket `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address());
  }

void
Listen_Socket::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& driver = this->do_abstract_socket_lock_driver(io_lock);

    for(;;) {
      // Try getting a connection.
      ::sockaddr_in6 sa;
      ::socklen_t salen = sizeof(sa);
      unique_posix_fd fd(::accept4(this->do_get_fd(), (::sockaddr*) &sa, &salen, SOCK_NONBLOCK));

      if(!fd) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
          break;

        POSEIDON_LOG_ERROR((
            "Error accepting TCP connection",
            "[`accept4()` failed: ${errno:full}]",
            "[TCP listen socket `$1` (class `$2`)]"),
            this, typeid(*this));

        // Errors are ignored.
        continue;
      }

      try {
        // Use `TCP_NODELAY`. Errors are ignored.
        static constexpr int true_value = 1;
        ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &true_value, sizeof(int));

        // Accept the client socket. If a null pointer is returned, the accepted
        // socket will be closed immediately.
        this->m_taddr.set_addr(sa.sin6_addr);
        this->m_taddr.set_port(be16toh(sa.sin6_port));

        auto client = this->do_on_listen_new_client_opt(::std::move(this->m_taddr), ::std::move(fd));
        if(ROCKET_UNEXPECT(!client))
          continue;

        POSEIDON_LOG_INFO((
            "Accepted new TCP connection from `$3`",
            "[accepted socket `$4` (class `$5`)]",
            "[TCP listen socket `$1` (class `$2`)]"),
            this, typeid(*this), this->m_taddr, client, typeid(*client));

        driver.insert(client);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception thrown from `do_on_listen_new_client_opt()`: $3",
            "[TCP listen socket `$1` (class `$2`)]"),
            this, typeid(*this), stdex);
      }
    }

    POSEIDON_LOG_TRACE((
        "TCP listen socket `$1` (class `$2`): `do_abstract_socket_on_readable()` done"),
        this, typeid(*this));
  }

void
Listen_Socket::
do_abstract_socket_on_oob_readable()
  {
  }

void
Listen_Socket::
do_abstract_socket_on_writable()
  {
  }

void
Listen_Socket::
defer_accept(seconds timeout)
  {
    int value = clamp_cast<int>(timeout.count(), 0, INT_MAX);
    if(::setsockopt(this->do_get_fd(), IPPROTO_TCP, TCP_DEFER_ACCEPT, &value, sizeof(int)) != 0)
      POSEIDON_THROW((
          "Failed to set TCP accept timeout",
          "[`setsockopt()` failed: ${errno:full}]",
          "[TCP listen socket `$1` (class `$2`)]"),
          this, typeid(*this));
  }

}  // namespace poseidon
