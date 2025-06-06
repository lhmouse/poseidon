// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_HTTPS_CLIENT_
#define POSEIDON_EASY_EASY_HTTPS_CLIENT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/https_client_session.hpp"
namespace poseidon {

class Easy_HTTPS_Client
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using callback_type =
      ::rocket::shared_function<
        void (
          const shptr<HTTPS_Client_Session>&,  // session
          Abstract_Fiber&,  // fiber for current callback
          Easy_HTTP_Event,  // event type; see comments above constructor
          HTTP_Response_Headers&&,  // response status code and headers
          linear_buffer&&  // response payload body
        )>;

  private:
    callback_type m_callback;

    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(const shptr<HTTPS_Client_Session>& session, Abstract_Fiber& fiber,
    // Easy_HTTP_Event event, HTTP_Response_Headers&& resp, linear_buffer&&
    // data)`, where `session` is a pointer to a client socket object, and if
    // `event` is
    //  1) `easy_http_open`, then `data` is empty; or
    //  2) `easy_http_message`, then `req` and `data` are the headers and body
    //     of a response message, respectively; or
    //  3) `easy_http_close`, then `resp` is empty and `data` is the error
    //     description.
    // This client object stores a copy of the callback object, which is invoked
    // accordingly in the main thread. The callback object is never copied, and
    // is allowed to modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(callback_type::is_viable<xCallback>::value)>
    explicit Easy_HTTPS_Client(xCallback&& cb)
      :
        m_callback(forward<xCallback>(cb))
      { }

  public:
    Easy_HTTPS_Client(const Easy_HTTPS_Client&) = delete;
    Easy_HTTPS_Client& operator=(const Easy_HTTPS_Client&) & = delete;
    ~Easy_HTTPS_Client();

    // Initiates a new connection to the given address. `addr` shall specify the
    // host name and (optional) port number. User names, paths, query parameters
    // or fragments are not allowed. If no port number is given, 443 is implied.
    shptr<HTTPS_Client_Session>
    connect(chars_view addr);

    // Shuts down all connections.
    void
    close_all() noexcept;
  };

}  // namespace poseidon
#endif
