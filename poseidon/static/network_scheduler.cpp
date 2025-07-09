// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "network_scheduler.hpp"
#include "../socket/abstract_socket.hpp"
#include "../socket/ssl_socket.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <sys/epoll.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
namespace poseidon {
namespace {

thread_local wkptr<SSL_Socket> s_weak_ssl_socket;

}  // namespace

Network_Scheduler::
Network_Scheduler() noexcept
  {
  }

Network_Scheduler::
~Network_Scheduler()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
wkptr<Abstract_Socket>&
Network_Scheduler::
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
Network_Scheduler::
do_alpn_select_cb(::SSL* ssl, const unsigned char** out, unsigned char* outlen,
                  const unsigned char* in, unsigned int inlen, void* /*arg*/)
  {
    auto socket = s_weak_ssl_socket.lock();
    if(!socket)
      return SSL_TLSEXT_ERR_ALERT_FATAL;

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
          "Unhandled exception thrown from socket ALPN callback: $3",
          "[SSL socket `$1` (class `$2`)]"),
          socket, typeid(*socket), stdex);
    }

    if(resp_len == 0)
      return SSL_TLSEXT_ERR_NOACK;

    POSEIDON_LOG_TRACE(("Accepting ALPN protocol: $1"), socket->m_alpn_proto);
    *out = reinterpret_cast<const uint8_t*>(socket->m_alpn_proto.data());
    *outlen = static_cast<uint8_t>(resp_len);
    return SSL_TLSEXT_ERR_OK;
  }

uniptr_SSL_CTX
Network_Scheduler::
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
Network_Scheduler::
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
Network_Scheduler::
reload(const Config_File& conf_file)
  {
    // Read the epoll event buffer size from configuration.
    uint32_t event_buffer_size = static_cast<uint32_t>(conf_file.get_integer_opt(
                          &"network.poll.event_buffer_size", 10, 1048576).value_or(1024));

    // Read the throttle size from configuration. If the size of the sending
    // queue of a socket has exceeded this value, receiving events will be
    // suspended.
    uint32_t throttle_size = static_cast<uint32_t>(conf_file.get_integer_opt(
                          &"network.poll.throttle_size", 0, INT_MAX).value_or(1048576));

    // Read SSL settings.
    cow_string default_certificate = conf_file.get_string_opt(&"network.ssl.default_certificate").value_or(&"");
    cow_string default_private_key = conf_file.get_string_opt(&"network.ssl.default_private_key").value_or(&"");

    if((default_certificate != "") && (default_private_key == ""))
      POSEIDON_THROW((
          "`network.ssl.default_private_key` is missing",
          "[in configuration file '$1']"),
          conf_file.path());

    if((default_certificate == "") && (default_private_key != ""))
      POSEIDON_THROW((
          "`network.ssl.default_certificate` is missing",
          "[in configuration file '$1']"),
          conf_file.path());

    // The server SSL context is optional, and is created only if a certificate
    // is configured.
    uniptr_SSL_CTX server_ssl_ctx;
    if(default_certificate != "") {
      if(!server_ssl_ctx.reset(::SSL_CTX_new(::TLS_server_method())))
        POSEIDON_THROW((
            "Could not allocate server SSL context",
            "[`SSL_CTX_new()` failed]: $1"),
            ::ERR_reason_error_string(::ERR_get_error()));

      ::SSL_CTX_set_mode(server_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
      ::SSL_CTX_set_mode(server_ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

      if(!::SSL_CTX_use_certificate_chain_file(server_ssl_ctx, default_certificate.safe_c_str()))
        POSEIDON_THROW((
            "Could not load default server SSL certificate file '$3'",
            "[`SSL_CTX_use_certificate_chain_file()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_get_error()), conf_file.path(),
            default_certificate);

      if(!::SSL_CTX_use_PrivateKey_file(server_ssl_ctx, default_private_key.safe_c_str(), SSL_FILETYPE_PEM))
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

      ::SSL_CTX_set_alpn_select_cb(server_ssl_ctx, do_alpn_select_cb, this);
      ::SSL_CTX_set_verify(server_ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
    }

    // The client SSL context is always created for outgoing connections.
    uniptr_SSL_CTX client_ssl_ctx;
    if(!client_ssl_ctx.reset(::SSL_CTX_new(::TLS_client_method())))
      POSEIDON_THROW((
          "Could not allocate client SSL context: $1",
          "[`SSL_CTX_new()` failed]"),
          ::ERR_reason_error_string(::ERR_get_error()));

    ::SSL_CTX_set_mode(client_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
    ::SSL_CTX_set_mode(client_ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    cow_string trusted_ca_path = conf_file.get_string_opt(&"network.ssl.trusted_ca_path").value_or(&"");
    if(trusted_ca_path != "") {
      if(!::SSL_CTX_load_verify_locations(client_ssl_ctx, nullptr, trusted_ca_path.safe_c_str()))
        POSEIDON_THROW((
            "Could not trusted CA path",
            "[`SSL_CTX_load_verify_locations()` failed: $1]",
            "[in configuration file '$2']"),
            ::ERR_reason_error_string(::ERR_get_error()), conf_file.path());

      ::SSL_CTX_set_verify(client_ssl_ctx, SSL_VERIFY_PEER, nullptr);
    }

    if(::SSL_CTX_get_verify_mode(client_ssl_ctx) == SSL_VERIFY_NONE)
      POSEIDON_LOG_WARN((
          "CA certificate validation has been disabled. This configuration is not "
          "recommended for production use. Set `network.ssl.trusted_ca_path` in '$1' "
          "to enable it."),
          conf_file.path());

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_event_buffer_size = event_buffer_size;
    this->m_throttle_size = throttle_size;
    this->m_server_ssl_ctx.swap(server_ssl_ctx);
    this->m_client_ssl_ctx.swap(client_ssl_ctx);
  }

void
Network_Scheduler::
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

    ::epoll_event pev;
    ROCKET_ASSERT(this->m_event_buf.size() >= sizeof(pev));
    this->m_event_buf.getn(reinterpret_cast<char*>(&pev), sizeof(pev));
    lock.unlock();

    lock.lock(this->m_epoll_mutex);
    if(this->m_epoll_map_stor.size() == 0)
      return;

    auto socket = this->do_find_socket_nolock(static_cast<Abstract_Socket*>(pev.data.ptr)).lock();
    if(!socket)
      return;

    recursive_mutex::unique_lock io_lock(socket->m_sched_mutex);
    socket->m_scheduler = this;
    lock.unlock();

    if(auto ssl_socket = dynamic_pointer_cast<SSL_Socket>(socket))
      s_weak_ssl_socket = ssl_socket;

    try {
      if(pev.events & EPOLLERR) {
        // Deliver a shutdown notification and remove the socket. Error codes
        // are passed through the system `errno` variable.
        socket->m_state.store(socket_closed);
        ::socklen_t optlen = sizeof(int);
        ::getsockopt(socket->m_fd, SOL_SOCKET, SO_ERROR, &errno, &optlen);
        socket->do_abstract_socket_on_closed();
      }
      else {
        // `EPOLLOUT` delivers connection establishment notification, so it has
        // to be called first. Similarly, `EPOLLHUP` delivers connection closure
        // notification, so it has to be called last.
        if(pev.events & EPOLLOUT)
          socket->do_abstract_socket_on_writeable();

        if(pev.events & EPOLLIN)
          socket->do_abstract_socket_on_readable();

        if(pev.events & EPOLLHUP) {
          socket->m_state.store(socket_closed);
          errno = 0;
          socket->do_abstract_socket_on_closed();
        }
      }
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR(("Socket error: $1"), stdex);
      socket->quick_shut_down();
      pev.events |= EPOLLERR;
    }

    if(pev.events & (EPOLLERR | EPOLLHUP)) {
      ::epoll_ctl(this->m_epoll_fd, EPOLL_CTL_DEL, socket->m_fd, &pev);
      socket->m_scheduler = reinterpret_cast<Network_Scheduler*>(-7);
      return;
    }

    // When there are too many pending bytes, as a safety measure, EPOLLIN
    // notifications are disabled until some bytes can be transferred.
    bool should_throttle = socket->m_sched_write_queue.size() > throttle_size;
    if(socket->m_sched_throttled != should_throttle) {
      socket->m_sched_throttled = should_throttle;

      if(should_throttle)
        pev.events = EPOLLOUT;  // output-only, level-triggered
      else
        pev.events = EPOLLIN |  EPOLLOUT | EPOLLET;

      if(::epoll_ctl(this->m_epoll_fd, EPOLL_CTL_MOD, socket->m_fd, &pev) != 0)
        POSEIDON_LOG_FATAL((
            "Could not modify socket `$1` (class `$2`)",
            "[`epoll_ctl()` failed: ${errno:full}]"),
            socket, typeid(*socket));
    }

    POSEIDON_LOG_TRACE(("Socket `$1` (class `$2`) I/O complete"), socket, typeid(*socket));
    socket->m_scheduler = (Network_Scheduler*) 123456789;
  }

void
Network_Scheduler::
insert_weak(const shptr<Abstract_Socket>& socket)
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

    // Insert the socket for polling.
    ::epoll_event pev;
    pev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    pev.data.ptr = socket.get();
    if(::epoll_ctl(this->m_epoll_fd, EPOLL_CTL_ADD, socket->m_fd, &pev) != 0)
      POSEIDON_THROW((
          "Could not add socket `$1` (class `$2`)",
          "[`epoll_ctl()` failed: ${errno:full}]"),
          socket, typeid(*socket));

    this->do_find_socket_nolock(socket.get()) = socket;
    this->m_epoll_map_used ++;
  }

}  // namespace poseidon
