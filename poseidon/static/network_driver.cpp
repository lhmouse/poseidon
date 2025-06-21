// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "network_driver.hpp"
#include "../socket/abstract_socket.hpp"
#include "../socket/ssl_socket.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <rocket/once_flag.hpp>
#include <sys/epoll.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
namespace poseidon {

Network_Driver::
Network_Driver() noexcept
  {
  }

Network_Driver::
~Network_Driver()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
void
Network_Driver::
do_epoll_ctl(int op, Abstract_Socket* socket, uint32_t events)
  {
    ROCKET_ASSERT(this->m_epoll_fd);

    ::epoll_event event;
    event.events = events;
    event.data.ptr = socket;
    if(::epoll_ctl(this->m_epoll_fd, op, socket->fd(), &event) != 0) {
      // When adding a socket, if the operation has failed, an exception shall
      // be thrown. For the other operations, errors are ignored.
      if(op == EPOLL_CTL_ADD)
        POSEIDON_THROW((
            "Could not add socket `$1` (class `$2`)",
            "[`epoll_ctl()` failed: ${errno:full}]"),
            socket, typeid(*socket));

      POSEIDON_LOG_FATAL((
          "Could not modify socket `$1` (class `$2`)",
          "[`epoll_ctl()` failed: ${errno:full}]"),
          socket, typeid(*socket));
    }

    if((op == EPOLL_CTL_ADD) || (op == EPOLL_CTL_MOD))
      POSEIDON_LOG_TRACE((
          "Updated epoll flags for socket `$1` (class `$2`): $3$4$5"),
          socket, typeid(*socket),
          (event.events & EPOLLET)  ? " ET"  : "",
          (event.events & EPOLLIN)  ? " IN"  : "",
          (event.events & EPOLLOUT) ? " OUT" : "");
  }

POSEIDON_VISIBILITY_HIDDEN
wkptr<Abstract_Socket>&
Network_Driver::
do_find_socket_nolock(volatile Abstract_Socket* socket) noexcept
  {
    ROCKET_ASSERT(socket);

    // Keep the load factor no more than 0.5. The table shall not be empty.
    uint64_t dist = this->m_epoll_map_stor.size();
    ROCKET_ASSERT(dist != 0);
    ROCKET_ASSERT(this->m_epoll_map_used <= dist / 2);

    // Make a fixed-point value in the interval [0,1), and then multiply `dist` by
    // it to get an index in the middle.
    uint32_t ratio = (uint32_t) ((uintptr_t) socket / sizeof(void*)) * 0x9E3779B9U;
    auto begin = &(this->m_epoll_map_stor[0]);
    auto origin = begin + (ptrdiff_t) (dist * ratio >> 32);
    auto end = begin + this->m_epoll_map_stor.size();

    // HACK: Compare the socket pointer without tampering with the reference
    // counter. The pointer itself will never be dereferenced. It [should] be
    // reasonable to assume `shared_ptr` and `weak_ptr` have identical layout.
#define do_get_weak_(wptr)  (reinterpret_cast<const shptr<Abstract_Socket>&>(wptr).get())

    // Find an element using linear probing. If the socket is not found, a
    // reference to an empty element is returned.
    for(auto elem = origin;  elem != end;  ++elem)
      if((do_get_weak_(*elem) == nullptr) || (do_get_weak_(*elem) == socket))
        return *elem;

    for(auto elem = begin;  elem != origin;  ++elem)
      if((do_get_weak_(*elem) == nullptr) || (do_get_weak_(*elem) == socket))
        return *elem;

    __builtin_unreachable();
  }

POSEIDON_VISIBILITY_HIDDEN
int
Network_Driver::
do_alpn_select_cb(::SSL* ssl, const unsigned char** out, unsigned char* outlen,
                  const unsigned char* in, unsigned int inlen, void* arg)
  {
    auto socket = static_cast<SSL_Socket*>(arg);
    ROCKET_ASSERT(socket);

    if(socket->m_ssl != ssl)
      return SSL_TLSEXT_ERR_ALERT_FATAL;

    if(inlen == 0)
      return SSL_TLSEXT_ERR_NOACK;

    charbuf_256 resp;
    size_t resp_len = 0;
    cow_vector<charbuf_256> req;

    try {
      // Parse ALPN request.
      const unsigned char* inp = in;
      while(inp != in + inlen) {
        size_t len = *inp;
        inp ++;

        // If the input is invalid, then fail the connection.
        if(static_cast<size_t>(in + inlen - inp) < len)
          return SSL_TLSEXT_ERR_ALERT_FATAL;

        char* sreq = req.emplace_back().mut_data();
        ::memcpy(sreq, inp, len);
        sreq[len] = 0;
        inp += len;
        POSEIDON_LOG_TRACE(("Received ALPN protocol: $1"), sreq);
      }

      socket->do_on_ssl_alpn_request(resp, move(req));
      resp_len = ::strlen(resp.c_str());
      socket->m_alpn_proto = resp;
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Unhandled exception thrown from socket ALPN callback: $1",
          "[socket class `$2`]"),
          stdex, typeid(*socket));
    }

    if(resp_len == 0)
      return SSL_TLSEXT_ERR_NOACK;

    POSEIDON_LOG_TRACE(("Accepting ALPN protocol: $1"), socket->m_alpn_proto);
    *out = reinterpret_cast<const uint8_t*>(socket->m_alpn_proto.data());
    *outlen = static_cast<uint8_t>(resp_len);
    return SSL_TLSEXT_ERR_OK;
  }

uniptr_SSL_CTX
Network_Driver::
server_ssl_ctx() const
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    auto ptr = this->m_server_ssl_ctx.get();
    if(!ptr)
      POSEIDON_THROW(("Server SSL not configured"));

    // Increment the reference count of the SSL context, as configuration may
    // be reloaded once this function returns.
    ::SSL_CTX_up_ref(ptr);
    return uniptr_SSL_CTX(ptr);
  }

uniptr_SSL_CTX
Network_Driver::
client_ssl_ctx() const
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    auto ptr = this->m_client_ssl_ctx.get();
    if(!ptr)
      POSEIDON_THROW(("Client SSL not configured"));

    // Increment the reference count of the SSL context, as configuration may
    // be reloaded once this function returns.
    ::SSL_CTX_up_ref(ptr);
    return uniptr_SSL_CTX(ptr);
  }

void
Network_Driver::
reload(const Config_File& conf_file)
  {
    static ::rocket::once_flag s_openssl_init_once;
    s_openssl_init_once.call(::OPENSSL_init_ssl, OPENSSL_INIT_NO_ATEXIT, nullptr);

    // Parse new configuration. Default ones are defined here.
    int64_t event_buffer_size = 1024;
    int64_t throttle_size = 1048576;
    cow_string default_certificate, default_private_key, trusted_ca_path;
    uniptr_SSL_CTX server_ssl_ctx, client_ssl_ctx;

    // Read the event buffer size from configuration.
    auto conf_value = conf_file.query(&"network.poll.event_buffer_size");
    if(conf_value.is_integer())
      event_buffer_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.poll.event_buffer_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((event_buffer_size < 0x10) || (event_buffer_size > 0x7FFFF0))
      POSEIDON_THROW((
          "`network.poll.event_buffer_size` value `$1` out of range",
          "[in configuration file '$2']"),
          event_buffer_size, conf_file.path());

    // Read the throttle size from configuration.
    conf_value = conf_file.query(&"network.poll.throttle_size");
    if(conf_value.is_integer())
      throttle_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.poll.throttle_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((throttle_size < 0x100) || (throttle_size > 0x7FFFFFF0))
      POSEIDON_THROW((
          "`network.poll.throttle_size` value `$1` out of range",
          "[in configuration file '$2']"),
          throttle_size, conf_file.path());

    // Get the path to the default server certificate and private key.
    conf_value = conf_file.query(&"network.ssl.default_certificate");
    if(conf_value.is_string())
      default_certificate = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.ssl.default_certificate`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    conf_value = conf_file.query(&"network.ssl.default_private_key");
    if(conf_value.is_string())
      default_private_key = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.ssl.default_private_key`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    // Check for configuration errors.
    if(!default_certificate.empty() && default_private_key.empty())
      POSEIDON_THROW((
          "`network.ssl.default_private_key` missing",
          "[in configuration file '$1']"),
          conf_file.path());

    if(default_certificate.empty() && !default_private_key.empty())
      POSEIDON_THROW((
          "`network.ssl.default_private_key` missing",
          "[in configuration file '$1']"),
          conf_file.path());

    if(!default_certificate.empty() && !default_private_key.empty()) {
      // Create the server context.
      if(!server_ssl_ctx.reset(::SSL_CTX_new(::TLS_server_method())))
        POSEIDON_THROW((
            "Could not allocate server SSL context",
            "[`SSL_CTX_new()` failed]: $1"),
            ::ERR_reason_error_string(::ERR_get_error()));

      ::SSL_CTX_set_mode(server_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
      ::SSL_CTX_set_mode(server_ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

      // Load the certificate and private key.
      if(!::SSL_CTX_use_certificate_chain_file(server_ssl_ctx,
                                               default_certificate.safe_c_str()))
        POSEIDON_THROW((
            "Could not load default server SSL certificate file '$3'",
            "[`SSL_CTX_use_certificate_chain_file()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_get_error()), conf_file.path(),
            default_certificate);

      if(!::SSL_CTX_use_PrivateKey_file(server_ssl_ctx, default_private_key.safe_c_str(),
                                        SSL_FILETYPE_PEM))
        POSEIDON_THROW((
            "Could not load default server SSL private key file '$3'",
            "[`SSL_CTX_use_PrivateKey_file()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_get_error()), conf_file.path(),
            default_private_key);

      if(!::SSL_CTX_check_private_key(server_ssl_ctx))
        POSEIDON_THROW((
            "Error validating default server SSL certificate '$3' and SSL private key '$4'",
            "[`SSL_CTX_check_private_key()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_get_error()), conf_file.path(),
            default_certificate, default_private_key);

      // The session context ID is composed from the DNS name of the running
      // machine. This determines which SSL sessions can be reused to improve
      // performance.
      uint8_t sid_ctx[32];
      ::memset(sid_ctx, '*', sizeof(sid_ctx));
      ::gethostname((char*) sid_ctx, sizeof(sid_ctx));
      if(!::SSL_CTX_set_session_id_context(server_ssl_ctx, sid_ctx, sizeof(sid_ctx)))
        POSEIDON_THROW((
            "Could not set SSL session ID context",
            "[`SSL_set_session_id_context()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_get_error()), conf_file.path());

      ::SSL_CTX_set_verify(server_ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
    }

    // Create the client context, always.
    if(!client_ssl_ctx.reset(::SSL_CTX_new(::TLS_client_method())))
      POSEIDON_THROW((
          "Could not allocate client SSL context: $1",
          "[`SSL_CTX_new()` failed]"),
          ::ERR_reason_error_string(::ERR_get_error()));

    ::SSL_CTX_set_mode(client_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
    ::SSL_CTX_set_mode(client_ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    // Get the path to trusted CA certificates.
    conf_value = conf_file.query(&"network.ssl.trusted_ca_path");
    if(conf_value.is_string())
      trusted_ca_path = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `network.ssl.trusted_ca_path`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if(!trusted_ca_path.empty()) {
      // Load trusted CA certificates from the given directory.
      if(!::SSL_CTX_load_verify_locations(client_ssl_ctx, nullptr,
                                          trusted_ca_path.safe_c_str()))
        POSEIDON_THROW((
            "Could not set path to trusted CA certificates",
            "[`SSL_CTX_load_verify_locations()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_get_error()), conf_file.path());

      ::SSL_CTX_set_verify(client_ssl_ctx, SSL_VERIFY_PEER, nullptr);
    }
    else {
      // Don't verify certificates at all.
      POSEIDON_LOG_WARN((
          "CA certificate validation has been disabled. This configuration is not "
          "recommended for production use. Set `network.ssl.trusted_ca_path` in '$1' "
          "to enable it."),
          conf_file.path());

      ::SSL_CTX_set_verify(client_ssl_ctx, SSL_VERIFY_NONE, nullptr);
    }

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_event_buffer_size = static_cast<uint32_t>(event_buffer_size);
    this->m_throttle_size = static_cast<uint32_t>(throttle_size);
    this->m_server_ssl_ctx.swap(server_ssl_ctx);
    this->m_client_ssl_ctx.swap(client_ssl_ctx);
  }

void
Network_Driver::
thread_loop()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const uint32_t event_buffer_size = this->m_event_buffer_size;
    const uint32_t throttle_size = this->m_throttle_size;
    lock.unlock();

    lock.lock(this->m_epoll_mutex);
    if(ROCKET_UNEXPECT(!this->m_epoll_fd)) {
      // Safety is on.
      lock.unlock();
      ::sleep(1);
      return;
    }

    lock.lock(this->m_event_mutex);
    if(this->m_event_buf.size() == 0) {
      this->m_event_buf.reserve_after_end(event_buffer_size * sizeof(::epoll_event));
      size_t real_capacity = this->m_event_buf.capacity_after_end() / sizeof(::epoll_event);

      int res = ::epoll_wait(this->m_epoll_fd,
                             reinterpret_cast<::epoll_event*>(this->m_event_buf.mut_end()),
                             static_cast<int>(real_capacity), 5000);
      if(res <= 0) {
        POSEIDON_LOG_TRACE(("`epoll_wait()` returned nothing: ${errno:full}"));
        return;
      }

      POSEIDON_LOG_TRACE(("Collected $1 events from epoll"), res);
      this->m_event_buf.accept(static_cast<uint32_t>(res) * sizeof(::epoll_event));
    }

    ::epoll_event epoll_ev = { };
    ROCKET_ASSERT(this->m_event_buf.size() >= sizeof(epoll_ev));
    this->m_event_buf.getn(reinterpret_cast<char*>(&epoll_ev), sizeof(epoll_ev));
    lock.unlock();

    lock.lock(this->m_epoll_mutex);
    if(this->m_epoll_map_stor.size() == 0)
      return;

    auto socket = this->do_find_socket_nolock(static_cast<Abstract_Socket*>(epoll_ev.data.ptr)).lock();
    if(!socket)
      return;

    if(auto ssl_socket = dynamic_cast<SSL_Socket*>(socket.get()))
      ::SSL_CTX_set_alpn_select_cb(::SSL_get_SSL_CTX(ssl_socket->m_ssl),
                                   do_alpn_select_cb, ssl_socket);

    recursive_mutex::unique_lock io_lock(socket->m_io_mutex);
    socket->m_io_driver = this;
    lock.unlock();

    // If either `EPOLLHUP` or `EPOLLERR` is set, deliver a shutdown
    // notification and remove the socket. Error codes are passed through the
    // system `errno` variable.
    if(epoll_ev.events & EPOLLIN)
      try {
        socket->do_abstract_socket_on_readable(epoll_ev.events & EPOLLRDHUP);
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR(("Socket error: $1"), stdex);
        socket->quick_shut_down();
        epoll_ev.events |= EPOLLHUP;
      }

    if(epoll_ev.events & (EPOLLHUP | EPOLLERR))
      try {
        socket->m_state.store(socket_closed);

        if(epoll_ev.events & EPOLLERR) {
          ::socklen_t optlen = sizeof(int);
          ::getsockopt(socket->m_fd, SOL_SOCKET, SO_ERROR, &errno, &optlen);
        } else
          errno = 0;
        socket->do_abstract_socket_on_closed();

        this->do_epoll_ctl(EPOLL_CTL_DEL, socket.get(), 0);
        socket->m_io_driver = reinterpret_cast<Network_Driver*>(-9);
        return;
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR(("Socket error: $1"), stdex);
        this->do_epoll_ctl(EPOLL_CTL_DEL, socket.get(), 0);
        socket->m_io_driver = reinterpret_cast<Network_Driver*>(-5);
        return;
      }

    if(epoll_ev.events & EPOLLOUT)
      try {
        socket->do_abstract_socket_on_writeable();
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR(("Socket error: $1"), stdex);
        socket->quick_shut_down();
        epoll_ev.events |= EPOLLHUP;
      }

    // When there are too many pending bytes, as a safety measure, EPOLLIN
    // notifications are disabled until some bytes can be transferred.
    bool should_throttle = socket->m_io_write_queue.size() > throttle_size;
    if(socket->m_io_throttled != should_throttle) {
      socket->m_io_throttled = should_throttle;
      if(should_throttle)
        this->do_epoll_ctl(EPOLL_CTL_MOD, socket.get(), EPOLLOUT);
      else
        this->do_epoll_ctl(EPOLL_CTL_MOD, socket.get(), EPOLLIN | EPOLLRDHUP | EPOLLOUT | EPOLLET);
    }

    POSEIDON_LOG_TRACE(("Socket `$1` (class `$2`) I/O complete"), socket, typeid(*socket));
    socket->m_io_driver = (Network_Driver*) 123456789;
  }

void
Network_Driver::
insert(const shptr<Abstract_Socket>& socket)
  {
    if(!socket)
      POSEIDON_THROW(("Null socket pointer not valid"));

    // Register the socket. Note exception safety.
    plain_mutex::unique_lock lock(this->m_epoll_mutex);

    if(!this->m_epoll_fd && !this->m_epoll_fd.reset(::epoll_create(100)))
      POSEIDON_THROW((
          "Could not allocate epoll object",
          "[`epoll_create()` failed: ${errno:full}]"));

    if(this->m_epoll_map_used >= this->m_epoll_map_stor.size() / 2) {
      // When the map is empty or the load factor would exceed 0.5, allocate a
      // larger map.
      uint32_t new_capacity = 17;
      for(size_t k = 0;  k != this->m_epoll_map_stor.size();  ++k)
        if(!this->m_epoll_map_stor[k].expired())
          new_capacity += 3;

      ::std::valarray<wkptr<Abstract_Socket>> old_map_stor(new_capacity);
      this->m_epoll_map_stor.swap(old_map_stor);
      this->m_epoll_map_used = 0;

      for(size_t k = 0;  k != old_map_stor.size();  ++k)
        if(!old_map_stor[k].expired()) {
          // Move this socket into the new map.
          // HACK: Get the socket pointer without tampering with the reference
          // counter. The pointer itself will never be dereferenced.
          this->do_find_socket_nolock(do_get_weak_(old_map_stor[k])).swap(old_map_stor[k]);
          this->m_epoll_map_used ++;
        }
    }

    // Insert the socket.
    this->do_epoll_ctl(EPOLL_CTL_ADD, socket.get(), EPOLLIN | EPOLLRDHUP | EPOLLOUT | EPOLLET);
    this->do_find_socket_nolock(socket.get()) = socket;
    this->m_epoll_map_used ++;
  }

}  // namespace poseidon
