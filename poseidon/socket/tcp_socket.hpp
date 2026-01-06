// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

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
    friend class Network_Scheduler;

    ::taxon::Value m_session_user_data;

  protected:
    // Takes ownership of an accepted socket. [server-side constructor]
    explicit
    TCP_Socket(unique_posix_fd&& fd);

    // Creates a socket for outgoing connections. [client-side constructor]
    TCP_Socket();

  protected:
    // These callbacks implement `Abstract_Socket`.
    virtual
    void
    do_abstract_socket_on_closed()
      override;

    virtual
    void
    do_abstract_socket_on_readable()
      override;

    virtual
    void
    do_abstract_socket_on_writeable()
      override;

    // This callback is invoked by the network thread after a full-duplex
    // connection has been established.
    // The default implementation merely prints a message.
    virtual
    void
    do_on_tcp_connected();

    // This callback is invoked by the network thread when some bytes have been
    // received, and is intended to be overriden by derived classes.
    // The argument contains all data that have been accumulated so far. Callees
    // should remove processed bytes.
    virtual
    void
    do_on_tcp_stream(linear_buffer& data, bool eof)
      = 0;

  public:
    TCP_Socket(const TCP_Socket&) = delete;
    TCP_Socket& operator=(const TCP_Socket&) & = delete;
    virtual ~TCP_Socket();

    // Get user-defined private data. This value is not used by the framework.
    const ::taxon::Value&
    session_user_data()
      const noexcept
      { return this->m_session_user_data;  }

    ::taxon::Value&
    mut_session_user_data()
      noexcept
      { return this->m_session_user_data;  }

    // Gets the maximum segment size (MSS) for outgoing packets.
    uint32_t
    max_segment_size()
      const;

    // Enqueues some bytes for sending.
    // If this function returns `true`, data will have been enqueued; however it
    // is not guaranteed that they will arrive at the destination host. If this
    // function returns `false`, the connection will have been closed.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    tcp_send(chars_view data);

    // Shuts the socket down gracefully. Errors during the shutdown operation
    // are ignored.
    // This function is thread-safe.
    bool
    tcp_shut_down()
      noexcept;
  };

}  // namespace poseidon
#endif
