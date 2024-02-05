// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_NETWORK_DRIVER_
#define POSEIDON_STATIC_NETWORK_DRIVER_

#include "../fwd.hpp"
#include "../third/openssl_fwd.hpp"
#include <valarray>
namespace poseidon {

class Network_Driver
  {
  private:
    mutable plain_mutex m_conf_mutex;
    uint32_t m_event_buffer_size = 0;
    uint32_t m_throttle_size = 0;
    uniptr_SSL_CTX m_server_ssl_ctx;
    uniptr_SSL_CTX m_client_ssl_ctx;

    mutable plain_mutex m_epoll_mutex;
    linear_buffer m_epoll_events;
    unique_posix_fd m_epoll_fd;
    uint32_t m_epoll_map_used = 0;
    ::std::valarray<wkptr<Abstract_Socket>> m_epoll_map_stor;

  public:
    // Constructs an empty driver.
    Network_Driver() noexcept;

  private:
    void
    do_epoll_ctl(int op, shptrR<Abstract_Socket> socket, uint32_t events);

    wkptr<Abstract_Socket>&
    do_linear_probe_socket_no_lock(const volatile Abstract_Socket* socket) noexcept;

    static
    int
    do_alpn_callback(::SSL* ssl, const uint8_t** out, uint8_t* outlen, const uint8_t* in,
                     unsigned int inlen, void* arg);

  public:
    Network_Driver(const Network_Driver&) = delete;
    Network_Driver& operator=(const Network_Driver&) & = delete;
    ~Network_Driver();

    // Gets the server SSL context for incoming connections, which is available
    // only if a certificate and a private key have been specified in 'main.conf'.
    // The certificate is sent to clients for verfication. If the server SSL
    // context is not available, an exception is thrown.
    // This function is thread-safe.
    uniptr_SSL_CTX
    server_ssl_ctx() const;

    // Gets the client SSL context for outgoing connections, which is always
    // available after `reload()`. If a path to trusted CA certificates is
    // specified in 'main.conf', server certificate verfication is enabled;
    // otherwise, a warning is printed and no verfication is performed.
    // This function is thread-safe.
    uniptr_SSL_CTX
    client_ssl_ctx() const;

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& conf_file);

    // Polls sockets.
    // This function should be called by the network thread repeatedly.
    void
    thread_loop();

    // Inserts a socket for polling. The network driver will hold a weak reference
    // to this socket.
    // This function is thread-safe.
    void
    insert(shptrR<Abstract_Socket> socket);
  };

}  // namespace poseidon
#endif
