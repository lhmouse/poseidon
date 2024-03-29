// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_SSL_CLIENT_
#define POSEIDON_EASY_EASY_SSL_CLIENT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../base/thunk.hpp"
#include "../socket/ssl_socket.hpp"
namespace poseidon {

class Easy_SSL_Client
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using thunk_type =
      thunk<
        shptrR<SSL_Socket>,  // client data socket
        Abstract_Fiber&,     // fiber for current callback
        Easy_Stream_Event,   // event type; see comments above constructor
        linear_buffer&,      // accumulative data that have been received
        int>;                // event code; see comments above constructor

  private:
    thunk_type m_thunk;

    shptr<Async_Connect> m_dns_task;
    struct X_Event_Queue;
    shptr<X_Event_Queue> m_queue;
    shptr<SSL_Socket> m_socket;

  public:
    // Constructs a client. The argument shall be an invocable object taking
    // `(shptrR<SSL_Socket> socket, Abstract_Fiber& fiber, Easy_Stream_Event
    // event, linear_buffer& data, int code)`, where `socket` is a pointer to
    // a client socket object, and if `event` is
    //  1) `easy_stream_open`, then `data` is empty; or
    //  2) `easy_stream_datahen `data` contains all data that have been
    //     received and have not been removed so far (this callback shall
    //     `.discard()` processed data from `data`, otherwise they will remain
    //     there for the next call); `code` is non-zero if the remote peer has
    //     closed the connection; or
    //  3) `easy_stream_closed`, then `data` is the error description and `code`
    //      is the system error number.
    // This client object stores a copy of the callback, which is invoked
    // accordingly in the main thread. The callback object is never copied,
    // and is allowed to modify itself.
    template<typename xCallback,
    ROCKET_ENABLE_IF(thunk_type::is_viable<xCallback>::value)>
    explicit Easy_SSL_Client(xCallback&& cb)
      :
        m_thunk(new_sh(forward<xCallback>(cb)))
      { }

    explicit Easy_SSL_Client(thunk_type::function_type* fptr)
      :
        m_thunk(fptr)
      { }

  public:
    Easy_SSL_Client(const Easy_SSL_Client&) = delete;
    Easy_SSL_Client& operator=(const Easy_SSL_Client&) & = delete;
    ~Easy_SSL_Client();

    // Initiates a new connection to the given address. `addr` shall specify the
    // host and port to connect, such as `localhost:12345`.
    void
    connect(chars_view addr);

    // Destroys the current connection without graceful shutdown. This function
    // should only be called after all data from the server have been read and
    // processed properly. If graceful shutdown is desired, `ssl_shut_down()`
    // shall be called first.
    void
    close() noexcept;

    // Gets the connection object.
    shptrR<SSL_Socket>
    socket_opt() const noexcept
      { return this->m_socket;  }

    // Gets the local address of this client for outgoing data. In case of an
    // error, `ipv6_unspecified` is returned.
    ROCKET_PURE
    const IPv6_Address&
    local_address() const noexcept;

    // Gets the remote or connected address of this client for outgoing data. In
    // case of errors, `ipv6_unspecified` is returned.
    ROCKET_PURE
    const IPv6_Address&
    remote_address() const noexcept;

    // Enqueues some bytes for sending.
    // If this function returns `true`, data will have been enqueued; however it
    // is not guaranteed that they will arrive at the destination host. If this
    // function returns `false`, the connection will have been closed.
    // If this function throws an exception, there is no effect.
    // This function is thread-safe.
    bool
    ssl_send(chars_view data);

    // Shuts the socket down gracefully. Errors during the shutdown operation
    // are ignored.
    // This function is thread-safe.
    bool
    ssl_shut_down() noexcept;
  };

}  // namespace poseidon
#endif
