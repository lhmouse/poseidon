// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_WSS_SERVER_
#define POSEIDON_EASY_EASY_WSS_SERVER_

#include "../fwd.hpp"
#include "enums.hpp"
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
        Easy_WS_Event,              // event type; see comments above constructor
        linear_buffer&&>;           // message payload

  private:
    thunk_type m_thunk;

    struct X_Client_Table;
    shptr<X_Client_Table> m_client_table;
    shptr<Listen_Socket> m_socket;

  public:
    // Constructs a server. The argument shall be an invocable object taking
    // `(shptrR<WSS_Server_Session> session, Abstract_Fiber& fiber,
    // Easy_WS_Event event, linear_buffer&& data)`, where `session` is a
    // pointer to a client session object, and if `event` is
    //  1) `easy_ws_open`, then `data` is the request URI; or
    //  2) `easy_ws_msg_text`, then `data` is a complete text message that has
    //     been received; or
    //  3) `easy_ws_msg_bin`, then `data` is a complete binary message that has
    //     been received; or
    //  4) `easy_ws_pong`, then `data` is a PONG notification that has been
    //     received, usually a copy of a previous PING notification; or
    //  5) `easy_ws_close`, then `data` is a string about the reason, such as
    //     `"1002: invalid opcode"`.
    // The server object owns all client socket objects. As a recommendation,
    // applications should store only `weak`s to client sockets, and call
    // `.lock()` as needed. This server object stores a copy of the callback,
    // which is invoked accordingly in the main thread. The callback object is
    // never copied, and is allowed to modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(thunk_type::is_invocable<xCallback>::value)>
    explicit Easy_WSS_Server(xCallback&& cb)
      :
        m_thunk(new_sh(forward<xCallback>(cb)))
      { }

    explicit Easy_WSS_Server(thunk_type::function_type* fptr)
      :
        m_thunk(fptr)
      { }

  public:
    Easy_WSS_Server(const Easy_WSS_Server&) = delete;
    Easy_WSS_Server& operator=(const Easy_WSS_Server&) & = delete;
    ~Easy_WSS_Server();

    // Starts listening the given address and port for incoming connections.
    void
    start(chars_view addr);

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
