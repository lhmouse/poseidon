// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_HTTPS_SERVER_
#define POSEIDON_EASY_EASY_HTTPS_SERVER_

#include "../fwd.hpp"
#include "../socket/https_server_session.hpp"
namespace poseidon {

class Easy_HTTPS_Server
  {
  public:
    // This is also the prototype of callbacks for the constructor.
    using thunk_type =
      thunk<
        shR<HTTPS_Server_Session>,  // server data socket
        Abstract_Fiber&,               // fiber for current callback
        Easy_HTTP_Event,               // event type; see comments above constructor
        HTTP_Request_Headers&&,        // request method, URI, and headers
        linear_buffer&&>;              // request payload body

  private:
    thunk_type m_thunk;

    struct X_Client_Table;
    sh<X_Client_Table> m_client_table;
    sh<Listen_Socket> m_socket;

  public:
    // Constructs a server. The argument shall be an invocable object taking
    // `(shR<HTTPS_Server_Session> session, Abstract_Fiber& fiber,
    // Easy_HTTP_Event event, HTTP_Request_Headers&& req, linear_buffer&&
    // data)`, where `session` is a pointer to a client session object, and if
    // `event` is
    //  1) `easy_http_open`, then `data` is empty; or
    //  2) `easy_http_message`, then `req` and `data` are the headers and body
    //     of a request message, respectively; or
    //  3) `easy_http_close`, then `resp` is empty and `data` is the error
    //     description.
    // The server object owns all client session objects. As a recommendation,
    // applications should store only `wkptr`s to client sessions, and call
    // `.lock()` as needed. This server object stores a copy of the callback
    // object, which is invoked accordingly in the main thread. The callback
    // object is never copied, and is allowed to modify itself.
    template<typename CallbackT,
    ROCKET_ENABLE_IF(thunk_type::is_invocable<CallbackT>::value)>
    explicit
    Easy_HTTPS_Server(CallbackT&& cb)
      :
        m_thunk(new_sh(forward<CallbackT>(cb)))
      { }

    explicit
    Easy_HTTPS_Server(thunk_type::function_type* fptr)
      :
        m_thunk(fptr)
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_HTTPS_Server);

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
    const Socket_Address&
    local_address() const noexcept;
  };

}  // namespace poseidon
#endif
