// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_UDP_SERVER_
#define POSEIDON_EASY_EASY_UDP_SERVER_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/udp_socket.hpp"
namespace poseidon {

class Easy_UDP_Server
  {
  public:
    // This is the user-defined callback, where `socket` points to an internal
    // server socket object, and `addr` and `data` are the source address and
    // payload of the current UDP packet, respectively. This server object
    // stores a copy of the callback, which is invoked accordingly in the main
    // thread. The callback object is never copied, and is allowed to modify
    // itself.
    using callback_type = ::rocket::shared_function<
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
    Easy_UDP_Server() noexcept = default;
    Easy_UDP_Server(const Easy_UDP_Server&) = delete;
    Easy_UDP_Server& operator=(const Easy_UDP_Server&) & = delete;
    ~Easy_UDP_Server();

    // Gets the local address of the listening socket. If the server is not
    // active, `ipv6_unspecified` is returned.
    const IPv6_Address&
    local_address() const noexcept;

    // Starts listening on the given address and port for incoming packets.
    shptr<UDP_Socket>
    start(const IPv6_Address& addr, const callback_type& callback);

    shptr<UDP_Socket>
    start(const cow_string& addr, const callback_type& callback);

    shptr<UDP_Socket>
    start(uint16_t port, const callback_type& callback);

    // Shuts down the socket, if any.
    void
    stop() noexcept;
  };

}  // namespace poseidon
#endif
