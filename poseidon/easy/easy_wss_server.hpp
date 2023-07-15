// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_WSS_SERVER_
#define POSEIDON_EASY_EASY_WSS_SERVER_

#include "../fwd.hpp"
#include "../socket/wss_server_session.hpp"
namespace poseidon {

class Easy_WSS_Server
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using thunk_type =
      thunk<
        shptrR<WSS_Server_Session>,  // server data socket
        Abstract_Fiber&,            // fiber for current callback
        WebSocket_Event,            // event type
        linear_buffer&&>;           // message payload

  private:
    thunk_type m_thunk;

    struct X_Client_Table;
    shptr<X_Client_Table> m_client_table;
    shptr<Listen_Socket> m_socket;

  public:
    // Constructs a server. The argument shall be an invocable object taking
    // `(shptrR<WSS_Server_Session> session, Abstract_Fiber& fiber,
    // WebSocket_Event event, linear_buffer&& data)`, where `session` is a
    // pointer to a client session object, and if `event` is
    //  1) `websocket_open`, then `data` is the request URI; or
    //  2) `websocket_text`/`websocket_binary`/`websocket_pong`, then `data` is a
    //     complete text/binary/pong message that has been received; or
    //  3) `websocket_closed`, then `data` is a string about the reason, such as
    //     `"1002: invalid opcode"`.
    // The server object owns all client socket objects. As a recommendation,
    // applications should store only `wkptr`s to client sockets, and call
    // `.lock()` as needed. This server object stores a copy of the callback,
    // which is invoked accordingly in the main thread. The callback object is
    // never copied, and is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_ENABLE_IF(thunk_type::is_invocable<CallbackT>::value)>
    explicit
    Easy_WSS_Server(CallbackT&& cb)
      : m_thunk(new_sh(::std::forward<CallbackT>(cb)))
      { }

    explicit
    Easy_WSS_Server(thunk_type::function_type* fptr)
      : m_thunk(fptr)
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_WSS_Server);

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
