// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_WS_CLIENT_
#define POSEIDON_EASY_EASY_WS_CLIENT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/ws_client_session.hpp"
namespace poseidon {

class Easy_WS_Client
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
    // 5) `easy_ws_close`, then `data` is a string about the reason, such
    //    as `"1002: invalid opcode"`.
    //
    // This client object stores a copy of the callback, which is invoked
    // accordingly in the main thread. The callback object is never copied, and
    // is allowed to modify itself.
    using callback_type = shared_function<
            void
             (const shptr<WS_Client_Session>& session,
              Abstract_Fiber& fiber,
              Easy_WS_Event event,
              linear_buffer&& data)>;

  private:
    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;

  public:
    Easy_WS_Client() noexcept = default;
    Easy_WS_Client(const Easy_WS_Client&) = delete;
    Easy_WS_Client& operator=(const Easy_WS_Client&) & = delete;
    ~Easy_WS_Client();

    // Initiates a new connection to the given address. `caddr` shall specify the
    // host name and (optional) port number to connect, and optional request path
    // and query parameters. User names or fragments are not allowed. If no port
    // number is given, 80 is implied.
    shptr<WS_Client_Session>
    connect(const cow_string& addr, const callback_type& callback);

    // Shuts down all connections.
    void
    close_all()
      noexcept;
  };

}  // namespace poseidon
#endif
