// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_WS_SERVER_
#define POSEIDON_EASY_EASY_WS_SERVER_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/ws_server_session.hpp"
namespace poseidon {

class Easy_WS_Server
  {
  public:
    // This is the user-defined callback, where `session` points to an internal
    // client session object, and if `event` is
    // 1) `easy_ws_open`, then `data` is the request URI; or
    // 2) `easy_ws_msg_text`, then `data` is a complete text message that has
    //    been received; or
    // 3) `easy_ws_msg_bin`, then `data` is a complete binary message that has
    //    been received; or
    // 4) `easy_ws_pong`, then `data` is a PONG notification that has been
    //    received, usually a copy of a previous PING notification; or
    // 5) `easy_ws_close`, then `data` is a string about the reason, such as
    //    `"1002: invalid opcode"`.
    //
    // The server object owns all client socket objects. As a recommendation,
    // applications should store only `weak`s to client sockets, and call
    // `.lock()` as needed. This server object stores a copy of the callback,
    // which is invoked accordingly in the main thread. The callback object is
    // never copied, and is allowed to modify itself.
    using callback_type = shared_function<
            void
             (const shptr<WS_Server_Session>& session,
              Abstract_Fiber& fiber,
              Easy_WS_Event event,
              linear_buffer&& data)>;

  private:
    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;
    shptr<TCP_Acceptor> m_acceptor;

  public:
    Easy_WS_Server() noexcept = default;
    Easy_WS_Server(const Easy_WS_Server&) = delete;
    Easy_WS_Server& operator=(const Easy_WS_Server&) & = delete;
    ~Easy_WS_Server();

    // Gets the local address of the listening socket. If the server is not
    // active, `ipv6_unspecified` is returned.
    const IPv6_Address&
    local_address()
      const noexcept;

    // Starts listening the given address and port for incoming connections.
    shptr<TCP_Acceptor>
    start(const IPv6_Address& addr, const callback_type& callback);

    shptr<TCP_Acceptor>
    start(const cow_string& addr, const callback_type& callback);

    shptr<TCP_Acceptor>
    start(uint16_t port, const callback_type& callback);

    // Shuts down the listening socket, if any. All existent clients are also
    // disconnected immediately.
    void
    stop()
      noexcept;
  };

}  // namespace poseidon
#endif
