// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "ssl_socket.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
namespace poseidon {

SSL_Socket::
SSL_Socket(unique_posix_fd&& fd, const SSL_CTX_ptr& ssl_ctx)
  : Abstract_Socket(::std::move(fd))
  {
    if(!ssl_ctx)
      POSEIDON_THROW((
          "Null SSL context pointer not valid",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this));

    if(!this->m_ssl.reset(::SSL_new(ssl_ctx)))
      POSEIDON_THROW((
          "Could not allocate server SSL structure",
          "[`SSL_new()` failed: $3]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this), ::ERR_reason_error_string(::ERR_peek_error()));

    if(!::SSL_set_fd(this->ssl(), this->do_get_fd()))
      POSEIDON_THROW((
          "Could not allocate SSL BIO for incoming connection",
          "[`SSL_set_fd()` failed: $3]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this), ::ERR_reason_error_string(::ERR_peek_error()));

    ::SSL_set_accept_state(this->ssl());
  }

SSL_Socket::
SSL_Socket(const SSL_CTX_ptr& ssl_ctx)
  : Abstract_Socket(SOCK_STREAM, IPPROTO_TCP)
  {
    if(!ssl_ctx)
      POSEIDON_THROW((
          "Null SSL context pointer not valid",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this));

    if(!this->m_ssl.reset(::SSL_new(ssl_ctx)))
      POSEIDON_THROW((
          "Could not allocate client SSL structure",
          "[`SSL_new()` failed: $3]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this), ::ERR_reason_error_string(::ERR_peek_error()));

    if(!::SSL_set_fd(this->ssl(), this->do_get_fd()))
      POSEIDON_THROW((
          "Could not allocate SSL BIO for outgoing connection",
          "[`SSL_set_fd()` failed: $3]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this), ::ERR_reason_error_string(::ERR_peek_error()));

    ::SSL_set_connect_state(this->ssl());
  }

SSL_Socket::
~SSL_Socket()
  {
    POSEIDON_LOG_INFO(("Destroying `$1` (class `$2`)"), this, typeid(*this));
  }

charbuf_256
SSL_Socket::
do_on_ssl_alpn_request(cow_vector<charbuf_256>&& protos)
  {
    for(const char* proto : protos)
      POSEIDON_LOG_DEBUG((
          "Ignoring ALPN protocol `$3` from socket `$1` (class `$2`)"),
          this, typeid(*this), proto);

    // Select no protocol.
    return "";
  }

void
SSL_Socket::
do_ssl_alpn_request(const charbuf_256* protos_opt, size_t protos_size)
  {
    if(protos_size == 1)
      return this->do_ssl_alpn_request(*protos_opt);

    // Generate the list of protocols in wire format.
    vector<uint8_t> pbuf;
    pbuf.reserve(256);

    for(size_t k = 0;  k != protos_size;  ++k) {
      // Encode this protocol name in the SSL ALPN wire format.
      const char* str = protos_opt[k];
      size_t len = ::strlen(str);
      if(len != 0) {
        ROCKET_ASSERT(len <= 255);
        pbuf.push_back((uint8_t) len);
        pbuf.insert(pbuf.end(), (const uint8_t*) str, (const uint8_t*) str + len);
      }
    }

    if(::SSL_set_alpn_protos(this->ssl(), pbuf.data(), (uint32_t) pbuf.size()) != 0)
      POSEIDON_THROW((
          "Failed to set ALPN protocol list",
          "[`SSL_set_alpn_protos()` failed]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this));
  }

void
SSL_Socket::
do_ssl_alpn_request(const cow_vector<charbuf_256>& protos)
  {
    this->do_ssl_alpn_request(protos.data(), protos.size());
  }

void
SSL_Socket::
do_ssl_alpn_request(initializer_list<charbuf_256> protos)
  {
    this->do_ssl_alpn_request(protos.begin(), protos.size());
  }

void
SSL_Socket::
do_ssl_alpn_request(const charbuf_256& proto)
  {
    static_vector<uint8_t, 256> pbuf;

    // Encode this protocol name in the SSL ALPN wire format.
    const char* str = proto;
    size_t len = ::strlen(str);
    if(len != 0) {
      ROCKET_ASSERT(len <= 255);
      pbuf.push_back((uint8_t) len);
      pbuf.insert(pbuf.end(), (const uint8_t*) str, (const uint8_t*) str + len);
    }

    if(::SSL_set_alpn_protos(this->ssl(), pbuf.data(), (uint32_t) pbuf.size()) != 0)
      POSEIDON_THROW((
          "Failed to set ALPN protocol list",
          "[`SSL_set_alpn_protos()` failed]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this));
  }

void
SSL_Socket::
do_abstract_socket_on_closed(int err)
  {
    POSEIDON_LOG_INFO((
        "SSL connection to `$3` closed: $4",
        "[SSL socket `$1` (class `$2`)]"),
        this, typeid(*this), this->remote_address(), format_errno(err));
  }

void
SSL_Socket::
do_abstract_socket_on_readable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_read_queue(io_lock);
    size_t old_size = queue.size();
    int ssl_err = 0;

    for(;;) {
      // Read bytes and append them to `queue`.
      queue.reserve_after_end(0xFFFFU);
      ssl_err = 0;
      size_t datalen;
      if(::SSL_read_ex(this->ssl(), queue.mut_end(), queue.capacity_after_end(), &datalen) <= 0) {
        ssl_err = ::SSL_get_error(this->ssl(), 0);

        // Check for EOF without a shutdown alert.
        if((ssl_err == SSL_ERROR_SYSCALL) && (errno == 0))
          ssl_err = SSL_ERROR_ZERO_RETURN;

#ifdef SSL_R_UNEXPECTED_EOF_WHILE_READING
        if((ssl_err == SSL_ERROR_SSL) && (ERR_GET_REASON(::ERR_peek_error()) == SSL_R_UNEXPECTED_EOF_WHILE_READING))
          ssl_err = SSL_ERROR_ZERO_RETURN;
#endif  // Open SSL 3.0

        if((ssl_err == SSL_ERROR_ZERO_RETURN) || (ssl_err == SSL_ERROR_WANT_READ) || (ssl_err == SSL_ERROR_WANT_WRITE))
          break;

        POSEIDON_LOG_ERROR((
            "Error reading SSL socket",
            "[`SSL_read_ex()` failed: SSL_get_error = `$3`, ERR_peek_error = `$4`, errno = `$5`]",
            "[SSL socket `$1` (class `$2`)]"),
            this, typeid(*this), ssl_err, ::ERR_reason_error_string(::ERR_peek_error()), format_errno());

        // The connection is now broken.
        this->quick_close();
        return;
      }

      // Accept incoming data.
      queue.accept(datalen);
    }

    if(old_size != queue.size()) {
      try {
        // Process received data.
        this->do_on_ssl_stream(queue);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception thrown from `do_on_ssl_stream()`: $1",
            "[socket class `$2`]"),
            stdex, typeid(*socket));

        this->quick_close();
        return;
      }
    }

    if(ssl_err == SSL_ERROR_ZERO_RETURN) {
      // If the end of stream has been reached, shut the connection down anyway.
      // Half-open connections are not supported.
      int alert = ::SSL_shutdown(this->ssl());
      POSEIDON_LOG_INFO(("Closing SSL connection: remote = $1, alert = $2"), this->remote_address(), alert);
      ::shutdown(this->do_get_fd(), SHUT_RDWR);
    }
  }

void
SSL_Socket::
do_abstract_socket_on_oob_readable()
  {
    char data;
    ::ssize_t io_result;

    // If there are no OOB data, `recv()` fails with `EINVAL`.
    io_result = ::recv(this->do_get_fd(), &data, 1, MSG_OOB);
    if(io_result > 0) {
      try {
        // Process received byte.
        this->do_on_ssl_oob_byte(data);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception thrown from `do_on_ssl_oob_byte()`: $1",
            "[socket class `$2`]"),
            stdex, typeid(*socket));

        this->quick_close();
        return;
      }
    }
  }

void
SSL_Socket::
do_abstract_socket_on_writable()
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);
    int ssl_err = 0;

    if(::SSL_do_handshake(this->ssl()) <= 0) {
      ssl_err = ::SSL_get_error(this->ssl(), 0);

      if((ssl_err == SSL_ERROR_WANT_READ) || (ssl_err == SSL_ERROR_WANT_WRITE))
        return;

      POSEIDON_LOG_ERROR((
          "Error performing SSL handshake",
          "[`SSL_do_handshake()` failed: SSL_get_error = `$3`, ERR_peek_error = `$4`, errno = `$5`]",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this), ssl_err, ::ERR_reason_error_string(::ERR_peek_error()), format_errno());

      // The connection is now broken.
      this->quick_close();
      return;
    }

    for(;;) {
      // Write bytes from `queue` and remove those written.
      if(queue.size() == 0)
        break;

      ssl_err = 0;
      size_t datalen;
      if(::SSL_write_ex(this->ssl(), queue.begin(), queue.size(), &datalen) <= 0) {
        ssl_err = ::SSL_get_error(this->ssl(), 0);

        if((ssl_err == SSL_ERROR_WANT_READ) || (ssl_err == SSL_ERROR_WANT_WRITE))
          break;

        POSEIDON_LOG_ERROR((
            "Error writing SSL socket",
            "[`SSL_write_ex()` failed: SSL_get_error = `$3`, ERR_peek_error = `$4`, errno = `$5`]",
            "[SSL socket `$1` (class `$2`)]"),
            this, typeid(*this), ssl_err, ::ERR_reason_error_string(::ERR_peek_error()), format_errno());

        // The connection is now broken.
        this->quick_close();
        return;
      }

      // Discard data that have been sent.
      queue.discard(datalen);
    }

    if(this->do_abstract_socket_change_state(socket_state_pending, socket_state_established)) {
      try {
        // Set the ALPN response for outgoing sockets. For incoming sockets, it is
        // set in the ALPN callback.
        const uint8_t* alpn_str;
        unsigned alpn_len;
        ::SSL_get0_alpn_selected(this->ssl(), &alpn_str, &alpn_len);

        this->m_alpn_proto.clear();
        this->m_alpn_proto.append((const char*) alpn_str, alpn_len);

        // Deliver the establishment notification.
        POSEIDON_LOG_DEBUG(("SSL connection established: remote = $1, alpn = $2"), this->remote_address(), this->m_alpn_proto);
        this->do_on_ssl_connected();
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception thrown from `do_on_ssl_connected()`: $1",
            "[socket class `$2`]"),
            stdex, typeid(*socket));

        this->quick_close();
        return;
      }
    }

    if(queue.empty() && this->do_abstract_socket_change_state(socket_state_closing, socket_state_closed)) {
      // If the socket has been marked closing and there are no more data, perform
      // complete shutdown.
      ::SSL_shutdown(this->ssl());
      ::shutdown(this->do_get_fd(), SHUT_RDWR);
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

void
SSL_Socket::
do_on_ssl_oob_byte(char data)
  {
    POSEIDON_LOG_INFO((
        "SSL connection received out-of-band data: $3 ($4)",
        "[SSL socket `$1` (class `$2`)]"),
        this, typeid(*this), (int) data, (char) data);
  }

const Socket_Address&
SSL_Socket::
remote_address() const noexcept
  {
    if(this->m_peername_ready.load())
      return this->m_peername;

    // Try getting the address now.
    static plain_mutex s_mutex;
    plain_mutex::unique_lock lock(s_mutex);

    if(this->m_peername_ready.load())
      return this->m_peername;

    ::sockaddr_in6 sa;
    ::socklen_t salen = sizeof(sa);
    if(::getpeername(this->do_get_fd(), (::sockaddr*) &sa, &salen) != 0)
      return ipv6_invalid;

    ROCKET_ASSERT(sa.sin6_family == AF_INET6);
    ROCKET_ASSERT(salen == sizeof(sa));

    if(sa.sin6_port == htobe16(0))
      return ipv6_unspecified;

    // Cache the address.
    this->m_peername.set_addr(sa.sin6_addr);
    this->m_peername.set_port(be16toh(sa.sin6_port));
    this->m_peername_ready.store(true);
    return this->m_peername;
  }

bool
SSL_Socket::
ssl_send(const char* data, size_t size)
  {
    if((data == nullptr) && (size != 0))
      POSEIDON_THROW((
          "Null data pointer",
          "[SSL socket `$1` (class `$2`)]"),
          this, typeid(*this));

    // If this socket has been marked closed, fail immediately.
    if(this->socket_state() >= socket_state_closing)
      return false;

    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);
    int ssl_err = 0;

    // Reserve backup space in case of partial writes.
    size_t nskip = 0;
    queue.reserve_after_end(size);

    if(queue.size() != 0) {
      // If there have been data pending, append new data to the end.
      ::memcpy(queue.mut_end(), data, size);
      queue.accept(size);
      return true;
    }

    for(;;) {
      // Try writing until the operation would block. This is essential for the
      // edge-triggered epoll to work reliably.
      if(nskip == size)
        break;

      ssl_err = 0;
      size_t datalen;
      int ret = ::SSL_write_ex(this->ssl(), data + nskip, size - nskip, &datalen);

      if(ret == 0) {
        ssl_err = ::SSL_get_error(this->ssl(), ret);

        if((ssl_err == SSL_ERROR_WANT_READ) || (ssl_err == SSL_ERROR_WANT_WRITE))
          break;

        POSEIDON_LOG_ERROR((
            "Error writing SSL socket",
            "[`SSL_write_ex()` failed: SSL_get_error = `$3`, ERR_peek_error = `$4`, errno = `$5`]",
            "[SSL socket `$1` (class `$2`)]"),
            this, typeid(*this), ssl_err, ::ERR_reason_error_string(::ERR_peek_error()), format_errno());

        // The connection is now broken.
        this->quick_close();
        return false;
      }

      // Discard data that have been sent.
      nskip += datalen;
    }

    // If the operation has completed only partially, buffer remaining data.
    // Space has already been reserved so this will not throw exceptions.
    ::memcpy(queue.mut_end(), data + nskip, size - nskip);
    queue.accept(size - nskip);
    return true;
  }

bool
SSL_Socket::
ssl_send_oob(char data) noexcept
  {
    return ::send(this->do_get_fd(), &data, 1, MSG_OOB) > 0;
  }

bool
SSL_Socket::
ssl_close() noexcept
  {
    recursive_mutex::unique_lock io_lock;
    auto& queue = this->do_abstract_socket_lock_write_queue(io_lock);

    // If there are data pending, mark this socket as being closed. If a full
    // connection has been established, wait until all pending data have been
    // sent. The connection should be closed thereafter.
    if(!queue.empty() && this->do_abstract_socket_change_state(socket_state_established, socket_state_closing))
      return true;

    // If there are no data pending, close it immediately.
    this->do_abstract_socket_set_state(socket_state_closed);
    ::SSL_shutdown(this->ssl());
    return ::shutdown(this->do_get_fd(), SHUT_RDWR) == 0;
  }

}  // namespace poseidon
