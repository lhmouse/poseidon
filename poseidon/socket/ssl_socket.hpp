// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_SSL_SOCKET_
#define POSEIDON_SOCKET_SSL_SOCKET_

#include "../fwd.hpp"
#include "enums.hpp"
#include "abstract_socket.hpp"
#include "../third/openssl_fwd.hpp"
namespace poseidon {

class SSL_Socket
  :
    public Abstract_Socket
  {
  private:
    friend class Network_Driver;

    uniptr_SSL m_ssl;
    cow_string m_alpn_proto;

    mutable atomic_relaxed<bool> m_peername_ready;
    mutable Socket_Address m_peername;

  protected:
    // Takes ownership of an accepted socket, using SSL configuration from
    // `driver`. [server-side constructor]
    SSL_Socket(unique_posix_fd&& fd, const Network_Driver& driver);

    // Creates a socket for outgoing connections, using SSL configuration
    // from `driver`. [client-side constructor]
    explicit SSL_Socket(const Network_Driver& driver);

  protected:
    // These callbacks implement `Abstract_Socket`.
    virtual
    void
    do_abstract_socket_on_closed() override;

    virtual
    void
    do_abstract_socket_on_readable() override;

    virtual
    void
    do_abstract_socket_on_oob_readable() override;

    virtual
    void
    do_abstract_socket_on_writable() override;

    // This callback is invoked by the network thread after a full-duplex
    // connection has been established.
    // The default implemention merely prints a message.
    virtual
    void
    do_on_ssl_connected();

    // This callback is invoked by the network thread when some bytes have been
    // received, and is intended to be overriden by derived classes.
    // The argument contains all data that have been accumulated so far. Callees
    // should remove processed bytes.
    virtual
    void
    do_on_ssl_stream(linear_buffer& data, bool eof) = 0;

    // This callback is invoked by the network thread when an out-of-band byte
    // has been received, and is intended to be overriden by derived classes.
    // The default implemention merely prints a message.
    virtual
    void
    do_on_ssl_oob_byte(char data);

    // For a server-side socket, this callback is invoked by the network thread
    // when ALPN has been requested by the client. This function should return
    // the name of protocol being selected. If an empty string is returned, no
    // ALPN protocol will be selected. The argument is the list of protocols
    // that have been offered by the client.
    // The default implemention returns an empty string.
    virtual
    char256
    do_on_ssl_alpn_request(vector<char256>&& protos);

    // For a client-side socket, this function offers a list of protocols to the
    // server. This function must be called before SSL negotiation, for example
    // inside the constructor of a derived class or just before assigning this
    // socket to the network driver. The argument is the list of protocols that
    // will be offered to the server. Empty protocol names are ignored. If the
    // list is empty, ALPN is not requested.
    void
    do_ssl_alpn_request(const char256* protos_opt, size_t protos_size);

    void
    do_ssl_alpn_request(const char256& proto);

  public:
    SSL_Socket(const SSL_Socket&) = delete;
    SSL_Socket& operator=(const SSL_Socket&) & = delete;
    virtual ~SSL_Socket();

    // Gets the remote or connected address of this socket. In case of errors,
    // `ipv6_invalid` is returned. The result is cached and will not
    // reflect changes that other APIs may have made.
    ROCKET_PURE
    const Socket_Address&
    remote_address() const noexcept;

    // Gets the maximum segment size (MSS) for outgoing packets.
    uint32_t
    max_segment_size() const;

    // Gets the protocol that has been selected by ALPN.
    // For a server-side socket, this string equals the result of a previous
    // `do_on_ssl_alpn_request()` callback. For a client-side socket, this
    // string is only available since the `do_on_ssl_connected()` callback.
    // If no ALPN protocol has been selected, an empty string is returned.
    cow_string
    alpn_protocol() const noexcept
      { return this->m_alpn_proto;  }

    // Enqueues some bytes for sending.
    // If this function returns `true`, data will have been enqueued; however it
    // is not guaranteed that they will arrive at the destination host. If this
    // function returns `false`, the connection will have been closed.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    ssl_send(chars_view data);

    // Sends an out-of-band byte. OOB bytes can be sent even when there are
    // pending normal data. This function never blocks. If the OOB byte cannot
    // be sent, `false` is returned and there is no effect.
    // This function is thread-safe.
    bool
    ssl_send_oob(char data) noexcept;

    // Shuts the socket down gracefully. Errors during the shutdown operation
    // are ignored.
    // This function is thread-safe.
    bool
    ssl_shut_down() noexcept;
  };

}  // namespace poseidon
#endif
