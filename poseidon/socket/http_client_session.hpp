// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_HTTP_CLIENT_SESSION_
#define POSEIDON_SOCKET_HTTP_CLIENT_SESSION_

#include "../fwd.hpp"
#include "tcp_socket.hpp"
namespace poseidon {

class HTTP_Client_Session
  : public TCP_Socket
  {
  private:
    friend class Network_Driver;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(HTTP_Client_Session);

  };

}  // namespace poseidon
#endif
