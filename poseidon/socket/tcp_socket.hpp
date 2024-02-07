// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_TCP_SOCKET_
#define POSEIDON_SOCKET_TCP_SOCKET_

#include "../fwd.hpp"
#include "enums.hpp"
#include "abstract_socket.hpp"
namespace poseidon {

class TCP_Socket
  :
    public Abstract_Socket
  {
  private:
    friend class Network_Driver;

    mutable atomic_relaxed<bool> m_peername_ready;
    mutable Socket_Address m_peername;

  protected:
    // Takes ownership of an accepted socket. [server-side constructor]
    explicit TCP_Socket(unique_posix_fd&& fd);

    // Creates a socket for outgoing connections. [client-side constructor]
    TCP_Socket();

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
    do_on_tcp_connected();

    // This callback is invoked by the network thread when some bytes have been
    // received, and is intended to be overriden by derived classes.
    // The argument contains all data that have been accumulated so far. Callees
    // should remove processed bytes.
    virtual
    void
    do_on_tcp_stream(linear_buffer& data, bool eof) = 0;

    // This callback is invoked by the network thread when an out-of-band byte
    // has been received, and is intended to be overriden by derived classes.
    // The default implemention merely prints a message.
    virtual
    void
    do_on_tcp_oob_byte(char data);

  public:
    TCP_Socket(const TCP_Socket&) = delete;
    TCP_Socket& operator=(const TCP_Socket&) & = delete;
    virtual ~TCP_Socket();

    // Gets the remote or connected address of this socket. In case of errors,
    // `ipv6_invalid` is returned. The result is cached and will not
    // reflect changes that other APIs may have made.
    ROCKET_PURE
    const Socket_Address&
    remote_address() const noexcept;

    // Gets the maximum segment size (MSS) for outgoing packets.
    uint32_t
    max_segment_size() const;

    // Enqueues some bytes for sending.
    // If this function returns `true`, data will have been enqueued; however it
    // is not guaranteed that they will arrive at the destination host. If this
    // function returns `false`, the connection will have been closed.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    tcp_send(chars_view data);

    // Sends an out-of-band byte. OOB bytes can be sent even when there are
    // pending normal data. This function never blocks. If the OOB byte cannot
    // be sent, `false` is returned and there is no effect.
    // This function is thread-safe.
    bool
    tcp_send_oob(char data) noexcept;

    // Shuts the socket down gracefully. Errors during the shutdown operation
    // are ignored.
    // This function is thread-safe.
    bool
    tcp_shut_down() noexcept;
  };

}  // namespace poseidon
#endif
