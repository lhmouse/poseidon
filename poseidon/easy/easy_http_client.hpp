// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_HTTP_CLIENT_
#define POSEIDON_EASY_EASY_HTTP_CLIENT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/http_client_session.hpp"
namespace poseidon {

class Easy_HTTP_Client
  {
  public:
    // This is the user-defined callback, where `session` points to an internal
    // client session object, and if `event` is
    // 1) `easy_http_open`, then `data` is empty; or
    // 2) `easy_http_message`, then `req` and `data` are the headers and body
    //    of a response message, respectively; or
    // 3) `easy_http_close`, then `resp` is empty and `data` is the error
    //    description.
    //
    // This client object stores a copy of the callback object, which is invoked
    // accordingly in the main thread. The callback object is never copied, and
    // is allowed to modify itself.
    using callback_type = shared_function<
            void (
              const shptr<HTTP_Client_Session>&  session,
              Abstract_Fiber& fiber,
              Easy_HTTP_Event event,
              HTTP_S_Headers&& resp,
              linear_buffer&& data)>;

  private:
    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;

  public:
    Easy_HTTP_Client() noexcept = default;
    Easy_HTTP_Client(const Easy_HTTP_Client&) = delete;
    Easy_HTTP_Client& operator=(const Easy_HTTP_Client&) & = delete;
    ~Easy_HTTP_Client();

    // Initiates a new connection to the given address. `addr` shall specify the
    // host name and (optional) port number. User names, paths, query parameters
    // or fragments are not allowed. If no port number is given, 80 is implied.
    shptr<HTTP_Client_Session>
    connect(const cow_string& addr, const callback_type& callback);

    // Shuts down all connections.
    void
    close_all()
      noexcept;
  };

}  // namespace poseidon
#endif
