// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_WSS_CLIENT_
#define POSEIDON_EASY_EASY_WSS_CLIENT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/wss_client_session.hpp"
namespace poseidon {

class Easy_WSS_Client
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using callback_type =
      ::rocket::shared_function<
        void (
          const shptr<WSS_Client_Session>&,  // session
          Abstract_Fiber&,  // fiber for current callback
          Easy_WS_Event,  // event type; see comments above constructor
          linear_buffer&&  // message payload
        )>;

  private:
    callback_type m_callback;

    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(const shptr<WSS_Client_Session>& session, Abstract_Fiber& fiber,
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
    template<typename xCallback,
    ROCKET_ENABLE_IF(callback_type::is_viable<xCallback>::value)>
    explicit Easy_WSS_Client(xCallback&& cb)
      :
        m_callback(forward<xCallback>(cb))
      { }

  public:
    Easy_WSS_Client(const Easy_WSS_Client&) = delete;
    Easy_WSS_Client& operator=(const Easy_WSS_Client&) & = delete;
    ~Easy_WSS_Client();

    // Initiates a new connection to the given address. `caddr` shall specify the
    // host name and (optional) port number to connect, and optional request path
    // and query parameters. User names or fragments are not allowed. If no port
    // number is given, 443 is implied.
    shptr<WSS_Client_Session>
    connect(chars_view caddr);

    // Shuts down all connections.
    void
    close_all() noexcept;
  };

}  // namespace poseidon
#endif
