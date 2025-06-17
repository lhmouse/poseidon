// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "tcp_socket.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netinet/tcp.h>
namespace poseidon {

TCP_Socket::
TCP_Socket(unique_posix_fd&& fd)
  :
    Abstract_Socket(move(fd))
  {
  }

TCP_Socket::
TCP_Socket()
  :
    Abstract_Socket(SOCK_STREAM, IPPROTO_TCP)
  {
  }

TCP_Socket::
~TCP_Socket()
  {
  }

void
TCP_Socket::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_INFO((
        "TCP connection to `$3` closed: ${errno:full}",
        "[TCP socket `$1` (class `$2`)]"),
        this, typeid(*this), this->remote_address());
  }

void
TCP_Socket::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_read_queue(io_lock);

    for(;;) {
      queue.clear();
      queue.reserve_after_end(0xFFFF);
      ::ssize_t ior = ::recv(this->do_socket_fd(), queue.mut_end(),
                             queue.capacity_after_end(), 0);
      if(ior < 0) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
          return;

        POSEIDON_LOG_DEBUG((
            "TCP socket read error: ${errno:full}",
            "[TCP socket `$1` (class `$2`)]"),
            this, typeid(*this));

        // The connection is now broken.
        this->shut_down();
        return;
      }

      queue.accept(static_cast<size_t>(ior));
      bool eof = ior == 0;

      try {
        // Call the user-defined data callback.
        this->do_on_tcp_stream(queue, eof);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception: $3",
            "[TCP socket `$1` (class `$2`)]"),
            this, typeid(*this), stdex);

        // The connection is now broken.
        this->shut_down();
        return;
      }

      if(eof) {
        // Close the connection passively.
        POSEIDON_LOG_DEBUG(("Received EOF from `$1`"), this->remote_address());
        this->shut_down();
        return;
      }

      POSEIDON_LOG_TRACE(("TCP socket `$1` (class `$2`) IN"), this, typeid(*this));
    }
  }

void
TCP_Socket::
do_abstract_socket_on_writeable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);

    if(this->do_socket_test_change(socket_pending, socket_established))
      try {
        // Call the user-defined establishment callback.
        this->do_on_tcp_connected();
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception: $3",
            "[TCP socket `$1` (class `$2`)]"),
            this, typeid(*this), stdex);

        // The connection is now broken.
        this->shut_down();
        return;
      }

    for(;;) {
      if(queue.empty()) {
        if(!this->do_socket_test_change(socket_closing, socket_closed))
          return;

        // The socket state has been changed from CLOSING to CLOSED, so close
        // the connection.
        POSEIDON_LOG_DEBUG(("Sending EOF to `$1`"), this->remote_address());
        ::shutdown(this->do_socket_fd(), SHUT_RDWR);
        return;
      }

      ::ssize_t ior = ::send(this->do_socket_fd(), queue.begin(), queue.size(), 0);
      if(ior < 0) {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
          return;

        POSEIDON_LOG_DEBUG((
            "TCP socket write error: ${errno:full}",
            "[TCP socket `$1` (class `$2`)]"),
            this, typeid(*this));

        // The connection is now broken.
        this->shut_down();
        return;
      }

      // Discard sent data.
      queue.discard(static_cast<size_t>(ior));

      POSEIDON_LOG_TRACE(("TCP socket `$1` (class `$2`) OUT"), this, typeid(*this));
    }
  }

void
TCP_Socket::
do_on_tcp_connected()
  {
    POSEIDON_LOG_INFO((
        "TCP connection to `$3` established",
        "[TCP socket `$1` (class `$2`)]"),
        this, typeid(*this), this->remote_address());
  }

uint32_t
TCP_Socket::
max_segment_size() const
  {
    int optval;
    ::socklen_t optlen = sizeof(optval);
    if(::getsockopt(this->do_socket_fd(), IPPROTO_TCP, TCP_MAXSEG, &optval, &optlen) != 0)
      POSEIDON_THROW((
          "Could not get MSS value",
          "[`getsockopt()` failed: ${errno:full}]",
          "[TCP socket `$1` (class `$2`)]"),
          this, typeid(*this));

    ROCKET_ASSERT(optlen == sizeof(optval));
    return static_cast<uint32_t>(optval);
  }

bool
TCP_Socket::
tcp_send(chars_view data)
  {
    if(this->socket_state() >= socket_closing)
      return false;

    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);

    // Reserve storage for the sake of exception safety.
    queue.reserve_after_end(data.n);

    if(queue.empty() && (this->socket_state() == socket_established)) {
      // Send until the operation would block.
      chars_view window = data;
      for(;;) {
        if(window.n == 0)
          return true;

        ::ssize_t ior = ::send(this->do_socket_fd(), window.p, window.n, 0);
        if(ior < 0) {
          if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            // Stash remaining data, and wait for the next writability
            // notification. Storage has been reserved so this will not throw
            // any exceptions.
            ::memcpy(queue.mut_end(), window.p, window.n);
            queue.accept(window.n);
            return true;
          }

          POSEIDON_LOG_DEBUG((
              "TCP socket write error: ${errno:full}",
              "[TCP socket `$1` (class `$2`)]"),
              this, typeid(*this));

          // The connection is now broken.
          this->shut_down();
          return false;
        }

        // Discard sent data;
        window >>= static_cast<size_t>(ior);

        POSEIDON_LOG_TRACE(("TCP socket `$1` (class `$2`) W"), this, typeid(*this));
      }
    }
    else {
      // If a previous write operation would have blocked, append `data` to
      // `queue`, and wait for the next writability notification.
      ::memcpy(queue.mut_end(), data.p, data.n);
      queue.accept(data.n);
      return true;
    }
  }

bool
TCP_Socket::
tcp_shut_down() noexcept
  {
    if(this->socket_state() >= socket_closing)
      return false;

    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);

    if(queue.empty()) {
      // Close the connection immediately.
      this->shut_down();
      return true;
    }
    else {
      // If a previous write operation would have blocked, mark the socket to
      // be closed once all data have been sent. The socket state shall not go
      // backwards.
      return this->do_socket_test_change(socket_pending, socket_closing)
             || this->do_socket_test_change(socket_established, socket_closing);
    }
  }

}  // namespace poseidon
