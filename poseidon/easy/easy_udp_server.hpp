// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_UDP_SERVER_
#define POSEIDON_EASY_EASY_UDP_SERVER_

#include "../fwd.hpp"
#include "../socket/udp_socket.hpp"
namespace poseidon {

class Easy_UDP_Server
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using thunk_type =
      thunk<
        shptrR<UDP_Socket>,  // server data socket
        Abstract_Fiber&,     // fiber for current callback
        Socket_Address&&,    // address of incoming packet
        linear_buffer&&>;    // data of incoming packet

  private:
    thunk_type m_thunk;

    struct X_Packet_Queue;
    shptr<X_Packet_Queue> m_queue;
    shptr<UDP_Socket> m_socket;

  public:
    // Constructs a server. The argument shall be an invocable object taking
    // `(shptrR<UDP_Socket> socket, Abstract_Fiber& fiber, Socket_Address&& addr,
    // linear_buffer&& data)`, where `socket` is a pointer to the server socket,
    // and `addr` and `data` are the source address and payload of the current
    // UDP packet, respectively. This server object stores a copy of the
    // callback, which is invoked accordingly in the main thread. The callback
    // object is never copied, and is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_ENABLE_IF(thunk_type::is_invocable<CallbackT>::value)>
    explicit
    Easy_UDP_Server(CallbackT&& cb)
      : m_thunk(new_sh(::std::forward<CallbackT>(cb)))
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_UDP_Server);

    // Starts listening the given address and port for incoming packets.
    void
    start(const Socket_Address& addr);

    // Shuts down the socket, if any.
    void
    stop() noexcept;

    // Gets the bound address of this server for incoming packets. In case of
    // errors, `ipv6_invalid` is returned.
    ROCKET_PURE
    const Socket_Address&
    local_address() const noexcept;

    // Joins/leaves a multicast group.
    // `maddr` is the multicast group to join/leave, and must be a valid multicast
    // address. It is also used to determine which network interface to use. `ttl`
    // specifies the TTL of multicast packets. `loopback` specifies whether packets
    // should be sent back to the sender. `ifname_opt` should specify the name of a
    // network interface to use; by default, the second interface is used (the first
    // one is usually the loopback one, namely `lo`).
    // If these functions fail, an exception is thrown, and the state of this socket
    // is unspecified.
    // These functions are thread-safe.
    void
    join_multicast_group(const Socket_Address& maddr, uint8_t ttl, bool loopback, const char* ifname_opt = nullptr);

    void
    leave_multicast_group(const Socket_Address& maddr, const char* ifname_opt = nullptr);

    // Enqueues a packet for sending.
    // If this function returns `true`, data will have been enqueued; however it
    // is not guaranteed that they will arrive at the destination host. It should
    // also be noted that UDP packets may be truncated if they are too large, which
    // is not considered errors; overflown data are dropped silently.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    udp_send(const Socket_Address& addr, const char* data, size_t size);
  };

}  // namespace poseidon
#endif
