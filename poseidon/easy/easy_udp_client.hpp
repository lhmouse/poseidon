// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_UDP_CLIENT_
#define POSEIDON_EASY_EASY_UDP_CLIENT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/udp_socket.hpp"
namespace poseidon {

class Easy_UDP_Client
  {
  public:
    // This is the user-defined callback, where `socket` points to an internal
    // client socket, and `addr` and `data` are the source address and payload
    // of the current UDP packet, respectively. This client object stores a
    // copy of the callback, which is invoked accordingly in the main thread.
    // The callback object is never copied, and is allowed to modify itself.
    using callback_type = shared_function<
            void
             (const shptr<UDP_Socket>& session,
              Abstract_Fiber& fiber,
              IPv6_Address&& addr,
              linear_buffer&& data)>;

  private:
    struct X_Packet_Queue;
    shptr<X_Packet_Queue> m_queue;
    shptr<UDP_Socket> m_socket;

  public:
    Easy_UDP_Client() noexcept = default;
    Easy_UDP_Client(const Easy_UDP_Client&) = delete;
    Easy_UDP_Client& operator=(const Easy_UDP_Client&) & = delete;
    ~Easy_UDP_Client();

    // Gets the local address of the listening socket. If the client is not
    // active, `ipv6_unspecified` is returned.
    const IPv6_Address&
    local_address()
      const noexcept;

    // Creates a socket for sending data.
    shptr<UDP_Socket>
    start(const callback_type& callback);

    // Shuts down the socket, if any.
    void
    stop()
      noexcept;
  };

}  // namespace poseidon
#endif
