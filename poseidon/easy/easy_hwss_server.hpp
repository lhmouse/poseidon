// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_HWSS_SERVER_
#define POSEIDON_EASY_EASY_HWSS_SERVER_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/wss_server_session.hpp"
namespace poseidon {

class Easy_HWSS_Server
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using callback_type =
      ::rocket::shared_function<
        void (
          shptrR<WSS_Server_Session>,  // session
          Abstract_Fiber&,             // fiber for current callback
          Easy_HWS_Event,              // event type; see comments above constructor
          linear_buffer&&              // message payload
        )>;

  private:
    callback_type m_callback;

    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;
    shptr<TCP_Acceptor> m_acceptor;

  public:
    // Constructs a server. The argument shall be an invocable object taking
    // `(shptrR<WSS_Server_Session> session, Abstract_Fiber& fiber,
    // Easy_HWS_Event event, linear_buffer&& data)`, where `session` is a
    // pointer to a client session object, and if `event` is
    //  1) `easy_hws_open`, then `data` is the request URI; or
    //  2) `easy_hws_msg_text`, then `data` is a complete text message that has
    //     been received; or
    //  3) `easy_hws_msg_bin`, then `data` is a complete binary message that has
    //     been received; or
    //  4) `easy_hws_pong`, then `data` is a PONG notification that has been
    //     received, usually a copy of a previous PING notification; or
    //  5) `easy_hws_close`, then `data` is a string about the reason, such as
    //     `"1002: invalid opcode"`.
    // The server object owns all client socket objects. As a recommendation,
    // applications should store only `weak`s to client sockets, and call
    // `.lock()` as needed. This server object stores a copy of the callback,
    // which is invoked accordingly in the main thread. The callback object is
    // never copied, and is allowed to modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(callback_type::is_viable<xCallback>::value)>
    explicit Easy_HWSS_Server(xCallback&& cb)
      :
        m_callback(forward<xCallback>(cb))
      { }

  public:
    Easy_HWSS_Server(const Easy_HWSS_Server&) = delete;
    Easy_HWSS_Server& operator=(const Easy_HWSS_Server&) & = delete;
    ~Easy_HWSS_Server();

    // Starts listening the given address and port for incoming connections.
    shptr<TCP_Acceptor>
    start(chars_view addr);

    // Shuts down the listening socket, if any. All existent clients are also
    // disconnected immediately.
    void
    stop() noexcept;
  };

}  // namespace poseidon
#endif
