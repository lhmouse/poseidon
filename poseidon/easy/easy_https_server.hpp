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
    // This is also the prototype of callbacks for the constructor.
    using callback_type =
      ::rocket::shared_function<
        void (
          shptrR<HTTPS_Server_Session>,  // session
          Abstract_Fiber&,               // fiber for current callback
          Easy_HTTP_Event,               // event type; see comments above constructor
          HTTP_Request_Headers&&,        // request method, URI, and headers
          linear_buffer&&                // request payload body
        )>;

  private:
    callback_type m_callback;

    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;
    shptr<TCP_Acceptor> m_acceptor;

  public:
    // Constructs a server. The argument shall be an invocable object taking
    // `(shptrR<HTTPS_Server_Session> session, Abstract_Fiber& fiber,
    // Easy_HTTP_Event event, HTTP_Request_Headers&& req, linear_buffer&&
    // data)`, where `session` is a pointer to a client session object, and if
    // `event` is
    //  1) `easy_http_open`, then `data` is empty; or
    //  2) `easy_http_message`, then `req` and `data` are the headers and body
    //     of a request message, respectively; or
    //  3) `easy_http_close`, then `resp` is empty and `data` is the error
    //     description.
    // The server object owns all client session objects. As a recommendation,
    // applications should store only `weak`s to client sessions, and call
    // `.lock()` as needed. This server object stores a copy of the callback
    // object, which is invoked accordingly in the main thread. The callback
    // object is never copied, and is allowed to modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(callback_type::is_viable<xCallback>::value)>
    explicit Easy_HTTPS_Server(xCallback&& cb)
      :
        m_callback(forward<xCallback>(cb))
      { }

  public:
    Easy_HTTPS_Server(const Easy_HTTPS_Server&) = delete;
    Easy_HTTPS_Server& operator=(const Easy_HTTPS_Server&) & = delete;
    ~Easy_HTTPS_Server();

    // Starts listening the given address and port for incoming connections.
    shptr<TCP_Acceptor>
    start(const IPv6_Address& addr);

    shptr<TCP_Acceptor>
    start(cow_stringR addr);

    shptr<TCP_Acceptor>
    start_any(uint16_t port);

    // Shuts down the listening socket, if any. All existent clients are also
    // disconnected immediately.
    void
    stop() noexcept;
  };

}  // namespace poseidon
#endif
