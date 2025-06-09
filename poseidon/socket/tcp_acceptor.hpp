// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_TCP_ACCEPTOR_
#define POSEIDON_SOCKET_TCP_ACCEPTOR_

#include "../fwd.hpp"
#include "enums.hpp"
#include "abstract_socket.hpp"
namespace poseidon {

class TCP_Acceptor
  :
    public Abstract_Socket
  {
  private:
    friend class Network_Driver;

  protected:
    // Creates a TCP socket that is bound onto the given address, that accepts
    // either TCP or SSL connections.
    explicit TCP_Acceptor(const IPv6_Address& addr);

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
    do_abstract_socket_on_writable() override;

    // This callback is invoked by the network thread when a connection has been
    // received, and is intended to be overriden by derived classes. This function
    // should return a pointer to a socket object, constructed from the given FD.
    virtual
    shptr<Abstract_Socket>
    do_accept_socket_opt(const IPv6_Address& addr, unique_posix_fd&& fd) = 0;

  public:
    TCP_Acceptor(const TCP_Acceptor&) = delete;
    TCP_Acceptor& operator=(const TCP_Acceptor&) & = delete;
    virtual ~TCP_Acceptor();

    // Defers connection establishment until the given timeout.
    void
    defer_accept(seconds timeout);
  };

}  // namespace poseidon
#endif
