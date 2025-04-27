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
    // This is also the prototype of callbacks for the constructor.
    using callback_type =
      ::rocket::shared_function<
        void (
          shptrR<UDP_Socket>,  // session
          Abstract_Fiber&,     // fiber for current callback
          IPv6_Address&&,      // address of incoming packet
          linear_buffer&&      // data of incoming packet
        )>;

  private:
    callback_type m_callback;

    struct X_Packet_Queue;
    shptr<X_Packet_Queue> m_queue;
    shptr<UDP_Socket> m_socket;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(shptrR<UDP_Socket> socket, Abstract_Fiber& fiber, IPv6_Address&& addr,
    // linear_buffer&& data)`, where `socket` is a pointer to the client socket,
    // and `addr` and `data` are the source address and payload of the current
    // UDP packet, respectively. This client object stores a copy of the
    // callback, which is invoked accordingly in the main thread. The callback
    // object is never copied, and is allowed to modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(callback_type::is_viable<xCallback>::value)>
    explicit Easy_UDP_Client(xCallback&& cb)
      :
        m_callback(forward<xCallback>(cb))
      { }

  public:
    Easy_UDP_Client(const Easy_UDP_Client&) = delete;
    Easy_UDP_Client& operator=(const Easy_UDP_Client&) & = delete;
    ~Easy_UDP_Client();

    // Creates a socket for sending data.
    shptr<UDP_Socket>
    start();

    // Shuts down the socket, if any.
    void
    stop() noexcept;
  };

}  // namespace poseidon
#endif
