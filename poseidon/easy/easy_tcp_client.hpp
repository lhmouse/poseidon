// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TCP_CLIENT_
#define POSEIDON_EASY_EASY_TCP_CLIENT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/tcp_socket.hpp"
namespace poseidon {

class Easy_TCP_Client
  {
  public:
    // This is the user-defined callback, where `socket` points to an internal
    // client socket object, and if `event` is
    // 1) `easy_stream_open`, then `data` is empty; or
    // 2) `easy_stream_data`, then `data` contains all data that have been
    //    received and have not been removed so far (this callback shall
    //    `.discard()` processed data from `data`, otherwise they will remain
    //    there for the next call); `code` is non-zero if the remote peer has
    //    closed the connection; or
    // 3) `easy_stream_closed`, then `data` is the error description and `code`
    //    is the system error number.
    //
    // This client object stores a copy of the callback, which is invoked
    // accordingly in the main thread. The callback object is never copied,
    // and is allowed to modify itself.
    using callback_type =
      shared_function<
        void (
          const shptr<TCP_Socket>&,  // session
          Abstract_Fiber&,  // fiber for current callback
          Easy_Stream_Event,  // event type; see comments above constructor
          linear_buffer&,  // accumulative data that have been received
          int  // event code; see comments above constructor
        )>;

  private:
    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;

  public:
    Easy_TCP_Client() noexcept = default;
    Easy_TCP_Client(const Easy_TCP_Client&) = delete;
    Easy_TCP_Client& operator=(const Easy_TCP_Client&) & = delete;
    ~Easy_TCP_Client();

    // Initiates a new connection to the given address. `addr` shall specify the
    // host and port to connect, such as `localhost:12345`.
    shptr<TCP_Socket>
    connect(const cow_string& addr, const callback_type& callback);

    // Shuts down all connections.
    void
    close_all()
      noexcept;
  };

}  // namespace poseidon
#endif
