// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TCP_SERVER_
#define POSEIDON_EASY_EASY_TCP_SERVER_

#include "../fwd.hpp"
#include "../socket/socket_address.hpp"
#include "../socket/tcp_socket.hpp"
namespace poseidon {

class Easy_TCP_Server
  {
  private:
    struct X_Client_Table;

    shared_ptr<void> m_cb_obj;
    callback_thunk_ptr<shared_ptrR<TCP_Socket>, Connection_Event, linear_buffer&> m_cb_thunk;

    shared_ptr<X_Client_Table> m_client_table;
    shared_ptr<Listen_Socket> m_socket;

  public:
    // Constructs a server. The argument shall be an invocable object taking
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
    // The server object owns all client socket objects. As a recommendation,
    // applications should store only `weak_ptr`s to client sockets, and call
    // `.lock()` as needed. This server object stores a copy of the callback,
    // which is invoked accordingly in the main thread. The callback object is
    // never copied, and is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_DISABLE_IF(::std::is_same<::std::decay_t<CallbackT>, Easy_TCP_Server>::value)>
    explicit
    Easy_TCP_Server(CallbackT&& cb)
      : m_cb_obj(::std::make_shared<::std::decay_t<CallbackT>>(::std::forward<CallbackT>(cb))),
        m_cb_thunk(callback_thunk<::std::decay_t<CallbackT>>)  { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_TCP_Server);

    // Starts listening the given address and port for incoming connections.
    void
    start(const Socket_Address& addr);

    // Shuts down the listening socket, if any. All existent clients are also
    // disconnected immediately.
    void
    stop() noexcept;

    // Gets the bound address of this server for incoming connections. In case
    // of errors, `ipv6_invalid` is returned.
    ROCKET_PURE
    const Socket_Address&
    local_address() const noexcept;
  };

}  // namespace poseidon
#endif
