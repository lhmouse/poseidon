// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_LISTEN_SOCKET_
#define POSEIDON_SOCKET_LISTEN_SOCKET_

#include "../fwd.hpp"
#include "abstract_socket.hpp"
namespace poseidon {

class Listen_Socket
  : public Abstract_Socket
  {
  private:
    friend class Network_Driver;

    Socket_Address m_taddr;

  protected:
    // Creates a TCP socket that is bound onto the given address, that accepts
    // either TCP or SSL connections. [server-side constructor]
    explicit
    Listen_Socket(const Socket_Address& addr);

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

    // This callback is invoked by the network thread when a connection has been
    // received, and is intended to be overriden by derived classes. This function
    // should return a pointer to a socket object, constructed from the given FD.
    virtual
    shptr<Abstract_Socket>
    do_on_listen_new_client_opt(Socket_Address&& addr, unique_posix_fd&& fd) = 0;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Listen_Socket);

    // Defers connection establishment until the given timeout.
    void
    defer_accept(seconds timeout);
  };

}  // namespace poseidon

#endif
