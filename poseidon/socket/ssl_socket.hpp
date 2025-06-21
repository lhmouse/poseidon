// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_SSL_SOCKET_
#define POSEIDON_SOCKET_SSL_SOCKET_

#include "../fwd.hpp"
#include "enums.hpp"
#include "abstract_socket.hpp"
#include "../details/openssl_fwd.hpp"
namespace poseidon {

class SSL_Socket
  :
    public Abstract_Socket
  {
  private:
    friend class Network_Driver;

    uniptr_SSL m_ssl;
    charbuf_256 m_alpn_proto;
    ::taxon::Value m_session_user_data;

  protected:
    // Takes ownership of an accepted socket, using SSL configuration from
    // `driver`. [server-side constructor]
    SSL_Socket(unique_posix_fd&& fd, const Network_Driver& driver);

    // Creates a socket for outgoing connections, using SSL configuration
    // from `driver`. [client-side constructor]
    explicit
    SSL_Socket(const Network_Driver& driver);

  protected:
    // These callbacks implement `Abstract_Socket`.
    virtual
    void
    do_abstract_socket_on_closed() override;

    virtual
    void
    do_abstract_socket_on_readable(bool rdhup) override;

    virtual
    void
    do_abstract_socket_on_writeable() override;

    // This callback is invoked by the network thread after a full-duplex
    // connection has been established.
    // The default implementation merely prints a message.
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

    // For a server-side socket, this callback is invoked by the network thread
    // when ALPN has been requested by the client. This function should write
    // the selected protocol into `res`. `protos` is the list of protocols that
    // have been offered by the client.
    // The default implementation does nothing.
    virtual
    void
    do_on_ssl_alpn_request(charbuf_256& res, cow_vector<charbuf_256>&& protos);

    // For a client-side socket, this function sets a list of protocols to offer
    // to the server. This function must be called before SSL negotiation, for
    // example inside the constructor of a derived class or just before assigning
    // this socket to the network driver. The argument is the list of protocols
    // that will be offered to the server. Empty protocol names are ignored. If
    // the list is empty, ALPN will not be negotiated.
    void
    do_ssl_alpn_request(const cow_vector<charbuf_256>& protos);

  public:
    SSL_Socket(const SSL_Socket&) = delete;
    SSL_Socket& operator=(const SSL_Socket&) & = delete;
    virtual ~SSL_Socket();

    // Get user-defined private data. This value is not used by the framework.
    const ::taxon::Value&
    session_user_data() const noexcept
      { return this->m_session_user_data;  }

    ::taxon::Value&
    mut_session_user_data() noexcept
      { return this->m_session_user_data;  }

    // Gets the maximum segment size (MSS) for outgoing packets.
    uint32_t
    max_segment_size() const;

    // Gets the protocol that has been selected by ALPN.
    // For a server-side socket, this string equals the result of a previous
    // `do_on_ssl_alpn_request()` callback. For a client-side socket, this
    // string is only available since the `do_on_ssl_connected()` callback.
    // If no ALPN protocol has been selected, an empty string is returned.
    const char*
    alpn_protocol() const noexcept
      { return this->m_alpn_proto.c_str();  }

    // Enqueues some bytes for sending.
    // If this function returns `true`, data will have been enqueued; however it
    // is not guaranteed that they will arrive at the destination host. If this
    // function returns `false`, the connection will have been closed.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    ssl_send(chars_view data);

    // Shuts the socket down gracefully. Errors during the shutdown operation
    // are ignored.
    // This function is thread-safe.
    bool
    ssl_shut_down() noexcept;
  };

}  // namespace poseidon
#endif
