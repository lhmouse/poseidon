// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_HTTPS_SERVER_
#define POSEIDON_EASY_EASY_HTTPS_SERVER_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/https_server_session.hpp"
namespace poseidon {

class Easy_HTTPS_Server
  {
  public:
    // This is the user-defined callback, where `session` points to an internal
    // client session object, and if `event` is
    // 1) `easy_http_open`, then `req` and `data` are empty; or
    // 2) `easy_http_message`, then `req` and `data` are the headers and body
    //    of a request message, respectively; or
    // 3) `easy_http_close`, then `req` is empty and `data` is the error
    //    description.
    //
    // The server object owns all client session objects. As a recommendation,
    // applications should store only `weak`s to client sessions, and call
    // `.lock()` as needed. This server object stores a copy of the callback
    // object, which is invoked accordingly in the main thread. The callback
    // object is never copied, and is allowed to modify itself.
    using callback_type = ::rocket::shared_function<
            void
             (const shptr<HTTPS_Server_Session>& session,
              Abstract_Fiber& fiber,
              Easy_HTTP_Event event,
              HTTP_Request_Headers&& req,
              linear_buffer&& data)>;

  private:
    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;
    shptr<TCP_Acceptor> m_acceptor;

  public:
    Easy_HTTPS_Server() noexcept = default;
    Easy_HTTPS_Server(const Easy_HTTPS_Server&) = delete;
    Easy_HTTPS_Server& operator=(const Easy_HTTPS_Server&) & = delete;
    ~Easy_HTTPS_Server();

    // Starts listening the given address and port for incoming connections.
    shptr<TCP_Acceptor>
    start(const IPv6_Address& addr, const callback_type& callback);

    shptr<TCP_Acceptor>
    start(const cow_string& addr, const callback_type& callback);

    shptr<TCP_Acceptor>
    start_any(uint16_t port, const callback_type& callback);

    // Shuts down the listening socket, if any. All existent clients are also
    // disconnected immediately.
    void
    stop() noexcept;
  };

}  // namespace poseidon
#endif
