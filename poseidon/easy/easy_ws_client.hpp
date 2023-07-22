// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_WS_CLIENT_
#define POSEIDON_EASY_EASY_WS_CLIENT_

#include "../fwd.hpp"
#include "../socket/ws_client_session.hpp"
namespace poseidon {

class Easy_WS_Client
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using thunk_type =
      thunk<
        shptrR<WS_Client_Session>,  // client data socket
        Abstract_Fiber&,            // fiber for current callback
        Easy_Socket_Event,          // event type; see comments above constructor
        linear_buffer&&>;           // message payload

  private:
    thunk_type m_thunk;

    shptr<Async_Connect> m_dns_task;
    struct X_Event_Queue;
    shptr<X_Event_Queue> m_queue;
    shptr<WS_Client_Session> m_session;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(shptrR<WS_Client_Session> session, Abstract_Fiber& fiber,
    // Easy_Socket_Event event, linear_buffer&& data)`, where `session` is a
    // pointer to a client session object, and if `event` is
    //  1) `easy_socket_open`, then `data` is the request URI; or
    //  2) `easy_socket_msg_text`, then `data` is a complete text message that
    //     has been received; or
    //  3) `easy_socket_msg_bin`, then `data` is a complete binary message that
    //     has been received; or
    //  4) `easy_socket_pong`, then `data` is a PONG notification that has been
    //     received, usually a copy of a previous PING notification; or
    //  5) `easy_socket_close`, then `data` is a string about the reason, such
    //     as `"1002: invalid opcode"`.
    // This client object stores a copy of the callback, which is invoked
    // accordingly in the main thread. The callback object is never copied, and
    // is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_ENABLE_IF(thunk_type::is_invocable<CallbackT>::value)>
    explicit
    Easy_WS_Client(CallbackT&& cb)
      : m_thunk(new_sh(::std::forward<CallbackT>(cb)))  { }

    explicit
    Easy_WS_Client(thunk_type::function_type* fptr)
      : m_thunk(fptr)  { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_WS_Client);

    // Initiates a new connection to the given address. `uri` shall start with
    // `ws://`, followed by a host name, an optional port number, a request path,
    // and optional query parameters; user information and fragments are not
    // allowed. If no port number is specified, 80 is assumed.
    void
    connect(cow_stringR uri);

    // Destroys the current connection without graceful shutdown. This function
    // should only be called after all data from the server have been read and
    // processed properly. If graceful shutdown is desired, `ws_shut_down()`
    // shall be called first.
    void
    close() noexcept;

    // Gets the connection session object.
    shptrR<WS_Client_Session>
    session_opt() const noexcept
      { return this->m_session;  }

    // Gets the local address of this client for outgoing data. In case of an
    // error, `ipv6_unspecified` is returned.
    ROCKET_PURE
    const Socket_Address&
    local_address() const noexcept;

    // Gets the remote or connected address of this client for outgoing data. In
    // case of errors, `ipv6_unspecified` is returned.
    ROCKET_PURE
    const Socket_Address&
    remote_address() const noexcept;

    // Sends a data message or control frame to the other peer. `opcode` indicates
    // the type of the message.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    ws_send(WebSocket_OpCode opcode, chars_proxy data);

    // Sends a CLOSE frame with an optional error message, then shuts down the
    // connection. The reason string will be truncated to 123 bytes if it's too
    // long.
    // This function is thread-safe.
    bool
    ws_shut_down(uint16_t status = 1000, chars_proxy reason = "") noexcept;
  };

}  // namespace poseidon
#endif
