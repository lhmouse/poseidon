// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "network_driver.hpp"
#include "../socket/abstract_socket.hpp"
#include "../socket/ssl_socket.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <sys/epoll.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
namespace poseidon {

Network_Driver::
Network_Driver()
  {
    if(!this->m_epoll.reset(::epoll_create1(0)))
      POSEIDON_THROW((
          "Could not create epoll object",
          "[`epoll_create1()` failed: ${errno:full}]"));
  }

Network_Driver::
~Network_Driver()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
void
Network_Driver::
do_epoll_ctl(int op, shptrR<Abstract_Socket> socket, uint32_t events)
  {
    ::epoll_event event;
    event.events = events;
    event.data.ptr = socket.get();
    if(::epoll_ctl(this->m_epoll, op, socket->fd(), &event) != 0) {
      // When adding a socket, if the operation has failed, an exception shall be
      // thrown. For the other operations, errors are ignored.
      if(op == EPOLL_CTL_ADD)
        POSEIDON_THROW((
            "Could not add socket `$1` (class `$2`)",
            "[`epoll_ctl()` failed: ${errno:full}]"),
            socket, typeid(*socket));

      POSEIDON_LOG_ERROR((
          "Could not modify socket `$1` (class `$2`)",
          "[`epoll_ctl()` failed: ${errno:full}]"),
          socket, typeid(*socket));
    }

    if((op == EPOLL_CTL_ADD) || (op == EPOLL_CTL_MOD))
      POSEIDON_LOG_TRACE((
          "Updated epoll flags for socket `$1` (class `$2`): ET = $3, IN = $4, PRI = $5, OUT = $6"),
          socket, typeid(*socket), event.events / EPOLLET & 1, event.events / EPOLLIN & 1,
          event.events / EPOLLPRI & 1, event.events / EPOLLOUT & 1);
  }

POSEIDON_VISIBILITY_HIDDEN
wkptr<Abstract_Socket>&
Network_Driver::
do_linear_probe_socket_no_lock(const volatile Abstract_Socket* socket) noexcept
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

    ROCKET_UNREACHABLE();
  }

POSEIDON_VISIBILITY_HIDDEN
int
Network_Driver::
do_alpn_callback(::SSL* ssl, const uint8_t** outp, uint8_t* outn, const uint8_t* inp, unsigned inn, void* arg) noexcept
  {
    // Verify the socket.
    // These errors shouldn't happen unless we have really messed things up
    // e.g. when `arg` is a dangling pointer.
    auto socket = dynamic_cast<SSL_Socket*>(static_cast<Abstract_Socket*>(arg));
    if(!socket)
      return SSL_TLSEXT_ERR_ALERT_FATAL;

    if(socket->ssl() != ssl)
      return SSL_TLSEXT_ERR_ALERT_FATAL;

    try {
      cow_vector<char256> alpn_req;
      auto bp = inp;
      const auto ep = inp + inn;

      while(bp != ep) {
        size_t len = *bp;
        ++ bp;
        if((size_t) (ep - bp) < len)
          return SSL_TLSEXT_ERR_ALERT_FATAL;  // malformed

        char* str = alpn_req.emplace_back();
        ::memcpy(str, bp, len);
        str[len] = 0;
        bp += len;
        POSEIDON_LOG_TRACE(("Received ALPN protocol: $1"), str);
      }

      char256 alpn_resp = socket->do_on_ssl_alpn_request(::std::move(alpn_req));
      socket->m_alpn_proto.assign(alpn_resp);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR((
          "Unhandled exception thrown from socket ALPN callback: $1",
          "[socket class `$2`]"),
          stdex, typeid(*socket));
    }

    if(socket->m_alpn_proto.empty())  // no protocol
      return SSL_TLSEXT_ERR_NOACK;

    POSEIDON_LOG_TRACE(("Responding ALPN protocol: $1"), socket->m_alpn_proto);
    *outp = (const uint8_t*) socket->m_alpn_proto.data();
    *outn = (uint8_t) socket->m_alpn_proto.size();
    return SSL_TLSEXT_ERR_OK;
  }

SSL_CTX_ptr
Network_Driver::
default_server_ssl_ctx() const
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);

    if(!this->m_server_ssl_ctx)
      POSEIDON_LOG_WARN((
          "Server SSL context unavailable",
          "[certificate not configured in 'main.conf']"));

    return this->m_server_ssl_ctx;
  }

SSL_CTX_ptr
Network_Driver::
default_client_ssl_ctx() const
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);

    if(!this->m_client_ssl_ctx)
      POSEIDON_LOG_WARN((
          "Client SSL context unavailable",
          "[no configuration loaded]"));

    return this->m_client_ssl_ctx;
  }

void
Network_Driver::
reload(const Config_File& conf_file)
  {
    // Parse new configuration. Default ones are defined here.
    int64_t event_buffer_size = 1024;
    int64_t throttle_size = 1048576;
    cow_string default_certificate, default_private_key, trusted_ca_path;
    SSL_CTX_ptr server_ssl_ctx, client_ssl_ctx;

    // Read the event buffer size from configuration.
    auto conf_value = conf_file.query("network", "poll", "event_buffer_size");
    if(conf_value.is_integer())
      event_buffer_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.poll.event_buffer_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((event_buffer_size < 0x10) || (event_buffer_size > 0x7FFFF0))
      POSEIDON_THROW((
          "`network.poll.event_buffer_size` value `$1` out of range",
          "[in configuration file '$2']"),
          event_buffer_size, conf_file.path());

    // Read the throttle size from configuration.
    conf_value = conf_file.query("network", "poll", "throttle_size");
    if(conf_value.is_integer())
      throttle_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.poll.throttle_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((throttle_size < 0x100) || (throttle_size > 0x7FFFFFF0))
      POSEIDON_THROW((
          "`network.poll.throttle_size` value `$1` out of range",
          "[in configuration file '$2']"),
          throttle_size, conf_file.path());

    // Get the path to the default server certificate and private key.
    conf_value = conf_file.query("network", "ssl", "default_certificate");
    if(conf_value.is_string())
      default_certificate = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.ssl.default_certificate`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    conf_value = conf_file.query("network", "ssl", "default_private_key");
    if(conf_value.is_string())
      default_private_key = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.ssl.default_private_key`: expecting a `string`, got `$1`",
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
            ::ERR_reason_error_string(::ERR_peek_error()));

      ::SSL_CTX_set_mode(server_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
      ::SSL_CTX_set_mode(server_ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

      // Load the certificate and private key.
      if(!::SSL_CTX_use_certificate_chain_file(server_ssl_ctx, default_certificate.safe_c_str()))
        POSEIDON_THROW((
            "Could not load default server SSL certificate file '$3'",
            "[`SSL_CTX_use_certificate_chain_file()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_peek_error()), conf_file.path(), default_certificate);

      if(!::SSL_CTX_use_PrivateKey_file(server_ssl_ctx, default_private_key.safe_c_str(), SSL_FILETYPE_PEM))
        POSEIDON_THROW((
            "Could not load default server SSL private key file '$3'",
            "[`SSL_CTX_use_PrivateKey_file()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_peek_error()), conf_file.path(), default_private_key);

      if(!::SSL_CTX_check_private_key(server_ssl_ctx))
        POSEIDON_THROW((
            "Error validating default server SSL certificate '$3' and SSL private key '$4'",
            "[`SSL_CTX_check_private_key()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_peek_error()), conf_file.path(), default_certificate, default_private_key);

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
            ::ERR_reason_error_string(::ERR_peek_error()), conf_file.path());

      ::SSL_CTX_set_verify(server_ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
    }

    // Create the client context, always.
    if(!client_ssl_ctx.reset(::SSL_CTX_new(::TLS_client_method())))
      POSEIDON_THROW((
          "Could not allocate client SSL context: $1",
          "[`SSL_CTX_new()` failed]"),
          ::ERR_reason_error_string(::ERR_peek_error()));

    ::SSL_CTX_set_mode(client_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
    ::SSL_CTX_set_mode(client_ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    // Get the path to trusted CA certificates.
    conf_value = conf_file.query("network", "ssl", "trusted_ca_path");
    if(conf_value.is_string())
      trusted_ca_path = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_LOG_WARN((
          "Ignoring `network.ssl.trusted_ca_path`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if(!trusted_ca_path.empty()) {
      // Load trusted CA certificates from the given directory.
      if(!::SSL_CTX_load_verify_locations(client_ssl_ctx, nullptr, trusted_ca_path.safe_c_str()))
        POSEIDON_THROW((
            "Could not set path to trusted CA certificates",
            "[`SSL_CTX_load_verify_locations()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_peek_error()), conf_file.path());

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
    this->m_event_buffer_size = (uint32_t) event_buffer_size;
    this->m_throttle_size = (uint32_t) throttle_size;
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
    const auto server_ssl_ctx = this->m_server_ssl_ctx;
    lock.unlock();

    // Get an event from the queue.
    lock.lock(this->m_epoll_event_mutex);
    if(this->m_epoll_events.empty()) {
      // If the queue is empty, populate it with more events from the epoll.
      // Errors are ignored.
      this->m_epoll_events.reserve_after_end(event_buffer_size * sizeof(::epoll_event));
      int nevents = ::epoll_wait(this->m_epoll, (::epoll_event*) this->m_epoll_events.mut_end(), (int) event_buffer_size, 5000);
      if(nevents <= 0) {
        POSEIDON_LOG_DEBUG(("`epoll_wait()` wait: ${errno:full}"));
        return;
      }

      POSEIDON_LOG_TRACE(("Collected `$1` socket event(s) from epoll"), (uint32_t) nevents);
      this->m_epoll_events.accept((uint32_t) nevents * sizeof(::epoll_event));
    }

    ::epoll_event event;
    ROCKET_ASSERT(this->m_epoll_events.size() >= sizeof(event));
    this->m_epoll_events.getn((char*) &event, sizeof(event));
    lock.unlock();

    // Lock the socket.
    lock.lock(this->m_epoll_map_mutex);
    if(this->m_epoll_map_stor.size() == 0)
      return;

    auto socket = this->do_linear_probe_socket_no_lock(static_cast<Abstract_Socket*>(event.data.ptr)).lock();
    if(!socket) {
      POSEIDON_LOG_TRACE(("Socket expired: $1"), event.data.ptr);
      return;
    }

    recursive_mutex::unique_lock io_lock(socket->m_io_mutex);
    socket->m_io_driver = this;
    lock.unlock();

    if(server_ssl_ctx)
      ::SSL_CTX_set_alpn_select_cb(server_ssl_ctx, do_alpn_callback, static_cast<Abstract_Socket*>(socket.get()));

    if(event.events & (EPOLLHUP | EPOLLERR)) {
      POSEIDON_LOG_TRACE(("Socket `$1` (class `$2`): EPOLLHUP | EPOLLERR"), socket, typeid(*socket));
      try {
        socket->m_state.store(socket_closed);
        ::socklen_t optlen = sizeof(int);
        if(event.events & EPOLLERR)
          ::getsockopt(socket->m_fd, SOL_SOCKET, SO_ERROR, &errno, &optlen);
        else
          errno = 0;
        socket->do_abstract_socket_on_closed();
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR(("`do_abstract_socket_on_closed()` error: $1", "[socket class `$2`]"), stdex, typeid(*socket));
      }
      socket->m_io_driver = (Network_Driver*) 123456789;
      POSEIDON_LOG_TRACE(("Socket `$1` (class `$2`): EPOLLHUP | EPOLLERR done"), socket, typeid(*socket));
      return;
    }

#define do_handle_epoll_event_(EPOLL_EVENT, target_function)  \
    if(event.events & EPOLL_EVENT) {  \
      POSEIDON_LOG_TRACE(("Socket `$1` (class `$2`): " #EPOLL_EVENT), socket, typeid(*socket));  \
      try {  \
        socket->target_function();  \
      }  \
      catch(exception& stdex) {  \
        POSEIDON_LOG_ERROR(("`" #target_function "()` error: $1", "[socket class `$2`]"), stdex, typeid(*socket));  \
        socket->quick_close();  \
        socket->m_io_driver = (Network_Driver*) 123456789;  \
        return;  \
      }  \
      POSEIDON_LOG_TRACE(("Socket `$1` (class `$2`): " #EPOLL_EVENT " done"), socket, typeid(*socket));  \
    }

    // `EPOLLOUT` must be the very first event, as it also delivers connection
    // establishment notifications.
    do_handle_epoll_event_(EPOLLOUT, do_abstract_socket_on_writable)
    do_handle_epoll_event_(EPOLLIN, do_abstract_socket_on_readable)
    do_handle_epoll_event_(EPOLLPRI, do_abstract_socket_on_oob_readable)

    if(socket->m_io_throttled != (socket->m_io_write_queue.size() > throttle_size)) {
      // When there are too many outgoing bytes pending, mostly likely due to
      // network instability, as a safety measure, EPOLLIN notifications are
      // disabled until some bytes can be transferred.
      socket->m_io_throttled ^= true;
      this->do_epoll_ctl(EPOLL_CTL_MOD, socket, EPOLLOUT | socket->m_io_throttled * (EPOLLIN | EPOLLPRI | EPOLLET));

      POSEIDON_LOG_INFO((
          "Throttle socket `$1` (class `$2`): queue_size = `$3`, throttled = `$4`"),
          socket, typeid(*socket), socket->m_io_write_queue.size(), socket->m_io_throttled);
    }

    POSEIDON_LOG_TRACE(("Socket `$1` (class `$2`) I/O complete"), socket, typeid(*socket));
    socket->m_io_driver = (Network_Driver*) 123456789;
  }

void
Network_Driver::
insert(shptrR<Abstract_Socket> socket)
  {
    if(!socket)
      POSEIDON_THROW(("Null socket pointer not valid"));

    // Register the socket. Note exception safety.
    plain_mutex::unique_lock lock(this->m_epoll_map_mutex);

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

      for(size_t k = 0;  k != old_map_stor.size();  ++k) {
        // Expired sockets are ignored.
        if(old_map_stor[k].expired())
          continue;

        // Move this socket into the new map.
        // HACK: Get the socket pointer without tampering with the reference
        // counter. The pointer itself will never be dereferenced.
        this->do_linear_probe_socket_no_lock(do_get_weak_(old_map_stor[k])).swap(old_map_stor[k]);
        this->m_epoll_map_used ++;
      }
    }

    // Insert the socket.
    this->do_epoll_ctl(EPOLL_CTL_ADD, socket, EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLET);
    this->do_linear_probe_socket_no_lock(socket.get()) = socket;
    this->m_epoll_map_used ++;
  }

}  // namespace poseidon
