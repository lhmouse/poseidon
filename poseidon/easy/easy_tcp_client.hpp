// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TCP_CLIENT_
#define POSEIDON_EASY_EASY_TCP_CLIENT_

#include "../fwd.hpp"
#include "../socket/socket_address.hpp"
#include "../socket/tcp_socket.hpp"
namespace poseidon {

class Easy_TCP_Client
  {
  private:
    struct X_Event_Queue;

    shared_ptr<void> m_cb_obj;
    callback_thunk_ptr<shared_ptrR<TCP_Socket>, Connection_Event, linear_buffer&> m_cb_thunk;

    shared_ptr<X_Event_Queue> m_queue;
    shared_ptr<TCP_Socket> m_socket;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(shared_ptrR<TCP_Socket> socket, Connection_Event event,
    // linear_buffer& data)`, where `socket` is a pointer to a client socket
    // object, and if `event` is
    //  1) `connection_event_open`, then `data` is empty, or
    //  2) `connection_event_stream`, then `data` contains all data that have
    //     been received and have not been removed so far (this callback shall
    //     `.discard()` processed data from `data`, otherwise they will remain
    //     there for the next call), or
    //  3) `connection_event_closed`, then `data` is the error description in
    //     case of an error, or an empty string if no error has happened.
    // This client object stores a copy of the callback, which is invoked
    // accordingly in the main thread. The callback object is never copied,
    // and is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_DISABLE_IF(::std::is_same<::std::decay_t<CallbackT>, Easy_TCP_Client>::value)>
    explicit
    Easy_TCP_Client(CallbackT&& cb)
      : m_cb_obj(::std::make_shared<::std::decay_t<CallbackT>>(::std::forward<CallbackT>(cb))),
        m_cb_thunk(callback_thunk<::std::decay_t<CallbackT>>)
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_TCP_Client);

    // Initiates a new connection to the given address.
    void
    start(const Socket_Address& addr);

    // Destroys the current connection without graceful shutdown. This function
    // should only be called after all data from the server have been read and
    // processed properly. If graceful shutdown is desired, `tcp_shut_down()`
    // shall be called first.
    void
    stop() noexcept;

    // Gets the local address of this client for incoming data. In case of
    // errors, `ipv6_unspecified` is returned.
    ROCKET_PURE
    const Socket_Address&
    local_address() const noexcept;

    // Gets the remote or connected address of this client for outgoing data. In
    // case of errors, `ipv6_unspecified` is returned.
    ROCKET_PURE
    const Socket_Address&
    remote_address() const noexcept;

    // Enqueues some bytes for sending.
    // If this function returns `true`, data will have been enqueued; however it
    // is not guaranteed that they will arrive at the destination host. If this
    // function returns `false`, the connection will have been closed.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    tcp_send(const char* data, size_t size);

    // Shuts the socket down gracefully. Errors during the shutdown operation
    // are ignored.
    // This function is thread-safe.
    bool
    tcp_shut_down() noexcept;
  };

}  // namespace poseidon
#endif
