// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "tcp_acceptor.hpp"
#include "../static/network_driver.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netinet/tcp.h>
namespace poseidon {

TCP_Acceptor::
TCP_Acceptor(const IPv6_Address& addr)
  :
    Abstract_Socket(SOCK_STREAM, IPPROTO_TCP)
  {
    static constexpr int one = 1;
    ::setsockopt(this->do_socket_fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    ::sockaddr_in6 sa = { };
    sa.sin6_family = AF_INET6;
    sa.sin6_port = ROCKET_HTOBE16(addr.port());
    sa.sin6_addr = addr.addr();
    if(::bind(this->do_socket_fd(), reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) != 0)
      POSEIDON_THROW((
          "Could not bind TCP socket to address `$1`",
          "[`bind()` failed: ${errno:full}]"),
          addr);

    if(::listen(this->do_socket_fd(), SOMAXCONN) != 0)
      POSEIDON_THROW((
          "Could not set TCP to accept mode on `$1`",
          "[`listen()` failed: ${errno:full}]"),
          addr);

    POSEIDON_LOG_INFO((
        "TCP socket started listening on `$3`",
        "[TCP acceptor `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address());
  }

TCP_Acceptor::
~TCP_Acceptor()
  {
  }

void
TCP_Acceptor::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_INFO((
        "TCP socket stopped listening on `$3`: ${errno:full}",
        "[TCP acceptor `$1` (class `$2`)]"),
        this, typeid(*this), this->local_address());
  }

void
TCP_Acceptor::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& driver = this->do_abstract_socket_lock_driver(io_lock);

    for(;;) {
      ::sockaddr_in6 sa;
      ::socklen_t salen = sizeof(sa);
      int afd = ::accept4(this->do_socket_fd(), reinterpret_cast<::sockaddr*>(&sa),
                          &salen, SOCK_NONBLOCK);
      if(afd == -1) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
          return;

        POSEIDON_LOG_DEBUG((
            "TCP socket accept error: ${errno:full}",
            "[TCP acceptor `$1` (class `$2`)]"),
            this, typeid(*this));

        // For TCP accept sockets, errors are ignored.
        continue;
      }

      unique_posix_fd fd(afd);
      IPv6_Address addr(sa.sin6_addr, ROCKET_BETOH16(sa.sin6_port));

      static constexpr int one = 1;
      ::setsockopt(afd, IPPROTO_TCP, TCP_QUICKACK, &one, sizeof(one));

      try {
        // Call the user-defined accept callback. If a null pointer is returned,
        // the accepted socket will be closed immediately.
        auto client = this->do_accept_socket_opt(move(addr), move(fd));
        if(ROCKET_UNEXPECT(!client))
          continue;

        POSEIDON_LOG_INFO((
            "TCP socket accepted new connection from `$3`",
            "[TCP client socket `$4` (class `$5`)]",
            "[TCP acceptor `$1` (class `$2`)]"),
            this, typeid(*this), client->remote_address(), client, typeid(*client));

        driver.insert(client);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "TCP socket accept error: ${errno:full}",
            "[TCP acceptor `$1` (class `$2`)]"),
            this, typeid(*this));

        // For TCP accept sockets, errors are ignored.
        continue;
      }

      POSEIDON_LOG_TRACE(("TCP acceptor `$1` (class `$2`) IN"), this, typeid(*this));
    }
  }

void
TCP_Acceptor::
do_abstract_socket_on_writeable()
  {
  }

void
TCP_Acceptor::
defer_accept(seconds timeout)
  {
    int iv = clamp_cast<int>(timeout.count(), 0, INT_MAX);
    if(::setsockopt(this->do_socket_fd(), IPPROTO_TCP, TCP_DEFER_ACCEPT, &iv, sizeof(iv)) != 0)
      POSEIDON_THROW((
          "Could not set TCP deferred accept timeout to `$3`",
          "[`setsockopt()` failed: ${errno:full}]",
          "[TCP acceptor `$1` (class `$2`)]"),
          this, typeid(*this), timeout);
  }

}  // namespace poseidon
