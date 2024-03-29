// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_TCP_SERVER_
#define POSEIDON_EASY_EASY_TCP_SERVER_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../base/thunk.hpp"
#include "../socket/tcp_socket.hpp"
namespace poseidon {

class Easy_TCP_Server
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using thunk_type =
      thunk<
        shptrR<TCP_Socket>,  // server data socket
        Abstract_Fiber&,     // fiber for current callback
        Easy_Stream_Event,   // event type; see comments above constructor
        linear_buffer&,      // accumulative data that have been received
        int>;                // event code; see comments above constructor

  private:
    thunk_type m_thunk;

    struct X_Client_Table;
    shptr<X_Client_Table> m_client_table;
    shptr<Listen_Socket> m_socket;

  public:
    // Constructs a server. The argument shall be an invocable object taking
    // `(shptrR<TCP_Socket> socket, Abstract_Fiber& fiber, Easy_Stream_Event
    // event, linear_buffer& data, int code)`, where `socket` is a pointer to
    // a client socket object, and if `event` is
    //  1) `easy_stream_open`, then `data` is empty; or
    //  2) `easy_stream_data`, then `data` contains all data that have been
    //     received and have not been removed so far (this callback shall
    //     `.discard()` processed data from `data`, otherwise they will remain
    //     there for the next call); `code` is non-zero if the remote peer has
    //     closed the connection; or
    //  3) `easy_stream_close`, then `data` is the error description and `code`
    //     is the system error number.
    // The server object owns all client socket objects. As a recommendation,
    // applications should store only `weak`s to client sockets, and call
    // `.lock()` as needed. This server object stores a copy of the callback,
    // which is invoked accordingly in the main thread. The callback object is
    // never copied, and is allowed to modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(thunk_type::is_viable<xCallback>::value)>
    explicit Easy_TCP_Server(xCallback&& cb)
      :
        m_thunk(new_sh(forward<xCallback>(cb)))
      { }

    explicit Easy_TCP_Server(thunk_type::function_type* fptr)
      :
        m_thunk(fptr)
      { }

  public:
    Easy_TCP_Server(const Easy_TCP_Server&) = delete;
    Easy_TCP_Server& operator=(const Easy_TCP_Server&) & = delete;
    ~Easy_TCP_Server();

    // Starts listening the given address and port for incoming connections.
    void
    start(chars_view addr);

    // Shuts down the listening socket, if any. All existent clients are also
    // disconnected immediately.
    void
    stop() noexcept;

    // Gets the bound address of this server for incoming connections. In case
    // of errors, `ipv6_invalid` is returned.
    ROCKET_PURE
    const IPv6_Address&
    local_address() const noexcept;
  };

}  // namespace poseidon
#endif
