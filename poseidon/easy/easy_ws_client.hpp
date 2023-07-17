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
        WebSocket_Event,            // event type
        linear_buffer&&>;           // message payload

  private:
    thunk_type m_thunk;

    struct X_Event_Queue;
    shptr<X_Event_Queue> m_queue;
    shptr<WS_Client_Session> m_session;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(shptrR<WS_Client_Session> session, Abstract_Fiber& fiber,
    // WebSocket_Event event, linear_buffer&& data)`, where `session` is a
    // pointer to a client session object, and if `event` is
    //  1) `websocket_open`, then `data` is the request URI; or
    //  2) `websocket_text`/`websocket_binary`/`websocket_pong`, then `data` is a
    //     complete text/binary/pong message that has been received; or
    //  3) `websocket_closed`, then `data` is a string about the reason, such as
    //     `"1002: invalid opcode"`.
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

    // Initiates a new connection to the given address.
    void
    connect(const Socket_Address& addr, cow_stringR uri = sref("/"));

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

    // Sends a text message to the other peer.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    ws_send_text(chars_proxy data);

    // Sends a binary message to the other peer.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    ws_send_binary(chars_proxy data);

    // Sends a PING frame. The payload string will be truncated to 125 bytes if
    // it's too long.
    bool
    ws_ping(chars_proxy data);

    // Sends a CLOSE frame with an optional error message, then shuts down the
    // connection. The reason string will be truncated to 123 bytes if it's too
    // long.
    // This function is thread-safe.
    bool
    ws_shut_down(uint16_t status = 1000, chars_proxy reason = "") noexcept;
  };

}  // namespace poseidon
#endif
