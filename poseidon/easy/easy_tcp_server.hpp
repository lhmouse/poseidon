// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TCP_SERVER_
#define POSEIDON_EASY_EASY_TCP_SERVER_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../socket/tcp_socket.hpp"
namespace poseidon {

class Easy_TCP_Server
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
    // 3) `easy_stream_close`, then `data` is the error description and `code`
    //    is the system error number.
    //
    // The server object owns all client socket objects. As a recommendation,
    // applications should store only `weak`s to client sockets, and call
    // `.lock()` as needed. This server object stores a copy of the callback,
    // which is invoked accordingly in the main thread. The callback object is
    // never copied, and is allowed to modify itself.
    using callback_type = shared_function<
            void
             (const shptr<TCP_Socket>& session,
              Abstract_Fiber& fiber,
              Easy_Stream_Event event,
              linear_buffer& data,
              int code)>;

  private:
    struct X_Session_Table;
    shptr<X_Session_Table> m_sessions;
    shptr<TCP_Acceptor> m_acceptor;

  public:
    Easy_TCP_Server() noexcept = default;
    Easy_TCP_Server(const Easy_TCP_Server&) = delete;
    Easy_TCP_Server& operator=(const Easy_TCP_Server&) & = delete;
    ~Easy_TCP_Server();

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
