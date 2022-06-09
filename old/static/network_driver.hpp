// This file is part of Poseidon.
// Copyleft 2020, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_NETWORK_DRIVER_
#define POSEIDON_STATIC_NETWORK_DRIVER_

#include "../fwd.hpp"

namespace poseidon {

class Network_Driver
  {
    POSEIDON_STATIC_CLASS_DECLARE(Network_Driver);

  public:
    // Reloads settings from main config.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    static
    void
    reload();

    // Adds a socket for polling.
    // The driver holds a reference-counted pointer to the socket. If it becomes a unique
    // reference, the socket is closed and deleted.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    static
    shared_ptr<Abstract_Socket>
    insert(const shared_ptr<Abstract_Socket>& sock);

    // Notifies the network thread about existence of writable data on a socket.
    // This is an internal function. You will not want to call it.
    // This function is thread-safe.
    static
    bool
    notify_writable_internal(const Abstract_Socket& ref) noexcept;
  };

}  // namespace poseidon

#endif