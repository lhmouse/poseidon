// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_WSS_CLIENT_
#define POSEIDON_EASY_EASY_WSS_CLIENT_

#include "../fwd.hpp"
#include "../socket/wss_client_session.hpp"
namespace poseidon {

class Easy_WSS_Client
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using thunk_type =
      thunk<
        shptrR<WSS_Client_Session>,  // client data socket
        Abstract_Fiber&,            // fiber for current callback
        Easy_WS_Event,              // event type; see comments above constructor
        linear_buffer&&>;           // message payload

  private:
    thunk_type m_thunk;

    shptr<Async_Connect> m_dns_task;
    struct X_Event_Queue;
    shptr<X_Event_Queue> m_queue;
    shptr<WSS_Client_Session> m_session;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(shptrR<WSS_Client_Session> session, Abstract_Fiber& fiber,
    // Easy_WS_Event event, linear_buffer&& data)`, where `session` is a
    // pointer to a client session object, and if `event` is
    //  1) `easy_ws_open`, then `data` is the request URI; or
    //  2) `easy_ws_msg_text`, then `data` is a complete text message that has
    //     been received; or
    //  3) `easy_ws_msg_bin`, then `data` is a complete binary message that has
    //     been received; or
    //  4) `easy_ws_pong`, then `data` is a PONG notification that has been
    //     received, usually a copy of a previous PING notification; or
    //  5) `easy_ws_close`, then `data` is a string about the reason, such
    //     as `"1002: invalid opcode"`.
    // This client object stores a copy of the callback, which is invoked
    // accordingly in the main thread. The callback object is never copied, and
    // is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_ENABLE_IF(thunk_type::is_invocable<CallbackT>::value)>
    explicit Easy_WSS_Client(CallbackT&& cb)
      :
        m_thunk(new_sh(forward<CallbackT>(cb)))
      { }

    explicit
    Easy_WSS_Client(thunk_type::function_type* fptr) :
        m_thunk(fptr)
      { }

  public:
    Easy_WSS_Client(const Easy_WSS_Client&) = delete;
    Easy_WSS_Client& operator=(const Easy_WSS_Client&) & = delete;
    ~Easy_WSS_Client();

    // Initiates a new connection to the given address. `caddr` shall specify the
    // host name and (optional) port number to connect, and optional request path
    // and query parameters. User names or fragments are not allowed. If no port
    // number is given, 443 is implied.
    void
    connect(chars_view caddr);

    // Destroys the current connection without graceful shutdown. This function
    // should only be called after all data from the server have been read and
    // processed properly. If graceful shutdown is desired, `wss_shut_down()`
    // shall be called first.
    void
    close() noexcept;

    // Gets the connection session object.
    shptrR<WSS_Client_Session>
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
    wss_send(Easy_WS_Event opcode, chars_view data);

    // Sends a CLOSE frame with an optional error message, then shuts down the
    // connection. The reason string will be truncated to 123 bytes if it's too
    // long.
    // This function is thread-safe.
    bool
    wss_shut_down(uint16_t status = 1000, chars_view reason = "") noexcept;
  };

}  // namespace poseidon
#endif
