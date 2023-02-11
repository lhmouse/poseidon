// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "listen_socket.hpp"
#include "../static/network_driver.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
namespace poseidon {

Listen_Socket::
Listen_Socket(const Socket_Address& addr)
  : Abstract_Socket(SOCK_STREAM, IPPROTO_TCP)
  {
    // Use `SO_REUSEADDR`. Errors are ignored.
    int ival = 1;
    ::setsockopt(this->fd(), SOL_SOCKET, SO_REUSEADDR, &ival, sizeof(ival));

    // Bind this socket onto `addr`.
    struct ::sockaddr_in6 sa;
    sa.sin6_family = AF_INET6;
    sa.sin6_port = htobe16(addr.port());
    sa.sin6_flowinfo = 0;
    sa.sin6_addr = addr.addr();
    sa.sin6_scope_id = 0;

    if(::bind(this->fd(), (const struct ::sockaddr*) &sa, sizeof(sa)) != 0)
      POSEIDON_THROW((
          "Failed to bind TCP socket onto `$4`",
          "[`bind()` failed: $3]",
          "[TCP socket `$1` (class `$2`)]"),
          this, typeid(*this), format_errno(), addr);

    if(::listen(this->fd(), SOMAXCONN) != 0)
      POSEIDON_THROW((
          "Failed to start listening on `$4`",
          "[`listen()` failed: $3]",
          "[TCP listen socket `$1` (class `$2`)]"),
          this, typeid(*this), format_errno(), addr);

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
do_abstract_socket_on_closed(int err)
  {
    POSEIDON_LOG_INFO((
        "TCP server stopped listening on `$3`: $4",
        "[TCP listen socket `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address(), format_errno(err));
  }

void
Listen_Socket::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& driver = this->do_abstract_socket_lock_driver(io_lock);

    for(;;) {
      // Try getting a connection.
      struct ::sockaddr_in6 sa;
      ::socklen_t salen = sizeof(sa);
      unique_posix_fd fd(::accept4(this->fd(), (::sockaddr*) &sa, &salen, SOCK_NONBLOCK));

      if(!fd) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
          break;

        POSEIDON_LOG_ERROR((
            "Error accepting TCP connection",
            "[`accept4()` failed: $3]",
            "[TCP listen socket `$1` (class `$2`)]"),
            this, typeid(*this), format_errno());

        // Errors are ignored.
        continue;
      }

      try {
        // Accept the client socket. If a null pointer is returned, the accepted
        // socket will be closed immediately.
        this->m_taddr.set_addr(sa.sin6_addr);
        this->m_taddr.set_port(be16toh(sa.sin6_port));
        auto client = this->do_on_listen_new_client_opt(::std::move(this->m_taddr), ::std::move(fd));
        if(!client)
          continue;

        POSEIDON_LOG_INFO((
            "Accepted new TCP connection `$3` (class `$4`)]",
            "[TCP listen socket `$1` (class `$2`)]"),
            this, typeid(*this), this->m_taddr, client, typeid(*client));

        driver.insert(client);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception thrown from `do_on_listen_new_client_opt()`: $1",
            "[socket class `$2`]"),
            stdex, typeid(*socket));
      }
    }
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

}  // namespace poseidon
