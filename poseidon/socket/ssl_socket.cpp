// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "ssl_socket.hpp"
#include "../static/network_scheduler.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
namespace poseidon {

SSL_Socket::
SSL_Socket(unique_posix_fd&& fd, const Network_Scheduler& scheduler)
  :
    Abstract_Socket(move(fd))
  {
    if(!this->m_ssl.reset(::SSL_new(scheduler.server_ssl_ctx())))
      POSEIDON_THROW((
          "Could not allocate SSL structure",
          "[`SSL_new()` failed: $1])"),
          ::ERR_reason_error_string(::ERR_get_error()));

    if(!::SSL_set_fd(this->m_ssl, this->do_socket_fd()))
      POSEIDON_THROW((
          "Could not allocate SSL BIO",
          "[`SSL_set_fd()` failed: $1]"),
          ::ERR_reason_error_string(::ERR_get_error()));

    ::SSL_set_accept_state(this->m_ssl);
  }

SSL_Socket::
SSL_Socket(const Network_Scheduler& scheduler)
  :
    Abstract_Socket(SOCK_STREAM, IPPROTO_TCP)
  {
    if(!this->m_ssl.reset(::SSL_new(scheduler.client_ssl_ctx())))
      POSEIDON_THROW((
          "Could not allocate SSL structure",
          "[`SSL_new()` failed: $1])"),
          ::ERR_reason_error_string(::ERR_get_error()));

    if(!::SSL_set_fd(this->m_ssl, this->do_socket_fd()))
      POSEIDON_THROW((
          "Could not allocate SSL BIO",
          "[`SSL_set_fd()` failed: $1]"),
          ::ERR_reason_error_string(::ERR_get_error()));

    ::SSL_set_connect_state(this->m_ssl);
  }

SSL_Socket::
~SSL_Socket()
  {
  }

void
SSL_Socket::
do_on_ssl_alpn_request(charbuf_256& res, cow_vector<charbuf_256>&& protos)
  {
    for(const auto& req : protos)
      POSEIDON_LOG_DEBUG((
          "Client offered ALPN protocol `$3`",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this), req);

    (void) res;
  }

void
SSL_Socket::
do_ssl_alpn_request(const cow_vector<charbuf_256>& protos)
  {
    linear_buffer bytes;
    for(const auto& req : protos) {
      size_t len = ::strlen(req.c_str());
      if(len != 0) {
        bytes.putc(static_cast<char>(len));
        bytes.putn(req.c_str(), len);
      }
    }

    if(::SSL_set_alpn_protos(this->m_ssl,
                             reinterpret_cast<const unsigned char*>(bytes.data()),
                             static_cast<unsigned>(bytes.size())) != 0)
      POSEIDON_THROW((
          "Could set ALPN protocols for negotiation",
          "[`SSL_set_alpn_protos()` failed]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this));
  }

void
SSL_Socket::
do_abstract_socket_on_closed()
  {
    POSEIDON_LOG_INFO((
        "SSL connection to `$3` closed: ${errno:full}",
        "[SSL socket `$1` (class `$2`)]"),
        this, typeid(*this), this->remote_address());
  }

void
SSL_Socket::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_read_queue(io_lock);

    for(;;) {
      queue.clear();
      queue.reserve_after_end(0xFFFF);
      size_t nread = queue.capacity_after_end();
      int ret = ::SSL_read_ex(this->m_ssl, queue.mut_end(), nread, &nread);
      if(ret <= 0)
        switch(::SSL_get_error(this->m_ssl, ret))
          {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            return;

          case SSL_ERROR_ZERO_RETURN:
            nread = 0;
            break;

          case SSL_ERROR_SSL:
#ifdef SSL_R_UNEXPECTED_EOF_WHILE_READING
            if(ERR_GET_REASON(::ERR_peek_error()) == SSL_R_UNEXPECTED_EOF_WHILE_READING) {
              nread = 0;
              break;
            }
#endif
            // fallthrough
          default:
            POSEIDON_LOG_DEBUG((
                "SSL socket read error: $3",
                "[system error: ${errno:full}]",
                "[SSL socket `$1` (class `$2`)]"),
                this, typeid(*this), ::ERR_reason_error_string(::ERR_get_error()));

            // The connection is now broken.
            this->quick_shut_down();
            return;
          }

      queue.accept(nread);
      bool eof = ret <= 0;

      try {
        // Call the user-defined data callback.
        this->do_on_ssl_stream(queue, eof);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception: $3",
            "[SSL socket `$1` (class `$2`)]"),
            this, typeid(*this), stdex);

        // The connection is now broken.
        this->quick_shut_down();
        return;
      }

      if(eof) {
        // Close the connection passively.
        this->quick_shut_down();
        return;
      }

      POSEIDON_LOG_TRACE(("SSL socket `$1` (class `$2`) IN"), this, typeid(*this));
    }
  }

void
SSL_Socket::
do_abstract_socket_on_writeable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);

    if(this->do_socket_test_change(socket_pending, socket_established))
      try {
        // Call the user-defined establishment callback.
        this->do_on_ssl_connected();
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception: $3",
            "[SSL socket `$1` (class `$2`)]"),
            this, typeid(*this), stdex);

        // The connection is now broken.
        this->quick_shut_down();
        return;
      }

    for(;;) {
      if(queue.empty()) {
        if(!this->do_socket_test_change(socket_closing, socket_closed))
          return;

        // The socket state has been changed from CLOSING to CLOSED, so close
        // the connection.
        POSEIDON_LOG_DEBUG(("Sending EOF to `$1`"), this->remote_address());
        ::SSL_shutdown(this->m_ssl);
        ::shutdown(this->do_socket_fd(), SHUT_RDWR);
        return;
      }

      size_t written = 0;
      int ret = ::SSL_write_ex(this->m_ssl, queue.begin(), queue.size(), &written);
      if(ret <= 0)
        switch(::SSL_get_error(this->m_ssl, ret))
          {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            return;

          default:
            POSEIDON_LOG_DEBUG((
                "SSL socket write error: $3",
                "[system error: ${errno:full}]",
                "[SSL socket `$1` (class `$2`)]"),
                this, typeid(*this), ::ERR_reason_error_string(::ERR_get_error()));

            // The connection is now broken.
            this->quick_shut_down();
            return;
          }

      // Discard sent data.
      queue.discard(written);

      POSEIDON_LOG_TRACE(("SSL socket `$1` (class `$2`) OUT"), this, typeid(*this));
    }
  }

void
SSL_Socket::
do_on_ssl_connected()
  {
    POSEIDON_LOG_INFO((
        "SSL connection to `$3` established",
        "[SSL socket `$1` (class `$2`)]"),
        this, typeid(*this), this->remote_address());
  }

uint32_t
SSL_Socket::
max_segment_size()
  const
  {
    int optval;
    ::socklen_t optlen = sizeof(optval);
    if(::getsockopt(this->do_socket_fd(), IPPROTO_TCP, TCP_MAXSEG, &optval, &optlen) != 0)
      POSEIDON_THROW((
          "Could not get MSS value",
          "[`getsockopt()` failed: ${errno:full}]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this));

    ROCKET_ASSERT(optlen == sizeof(optval));
    return static_cast<uint32_t>(optval);
  }

bool
SSL_Socket::
ssl_send(chars_view data)
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

        size_t written = 0;
        int ret = ::SSL_write_ex(this->m_ssl, window.p, window.n, &written);
        if(ret <= 0)
          switch(::SSL_get_error(this->m_ssl, ret))
            {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
              // Stash remaining data, and wait for the next writability
              // notification. Storage has been reserved so this will not throw
              // any exceptions.
              ::memcpy(queue.mut_end(), window.p, window.n);
              queue.accept(window.n);
              return true;

            default:
              POSEIDON_LOG_DEBUG((
                  "SSL socket write error: $3",
                  "[system error: ${errno:full}]",
                  "[SSL socket `$1` (class `$2`)]"),
                  this, typeid(*this), ::ERR_reason_error_string(::ERR_get_error()));

              // The connection is now broken.
              this->quick_shut_down();
              return false;
            }

        // Discard sent data.
        window >>= written;

        POSEIDON_LOG_TRACE(("SSL socket `$1` (class `$2`) W"), this, typeid(*this));
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
SSL_Socket::
ssl_shut_down()
  noexcept
  {
    if(this->socket_state() >= socket_closing)
      return false;

    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);

    if(queue.empty()) {
      // Close the connection immediately.
      ::SSL_shutdown(this->m_ssl);
      this->quick_shut_down();
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
