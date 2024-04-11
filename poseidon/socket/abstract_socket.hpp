// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_ABSTRACT_SOCKET_
#define POSEIDON_SOCKET_ABSTRACT_SOCKET_

#include "../fwd.hpp"
#include "enums.hpp"
#include "ipv6_address.hpp"
namespace poseidon {

class Abstract_Socket
  {
  private:
    friend class Network_Driver;

    unique_posix_fd m_fd;
    atomic_relaxed<Socket_State> m_state;
    bool m_io_throttled = false;  // protected by `m_io_mutex`
    char m_reserved_1;
    mutable atomic_relaxed<bool> m_sockname_ready;
    mutable IPv6_Address m_sockname;

    mutable recursive_mutex m_io_mutex;
    Network_Driver* m_io_driver;
    linear_buffer m_io_read_queue;
    linear_buffer m_io_write_queue;

  protected:
    // Takes ownership of an existent IPv6 socket. [server-side constructor]
    explicit Abstract_Socket(unique_posix_fd&& fd);

    // Creates a new non-blocking IPv6 socket. [client-side constructor]
    Abstract_Socket(int type, int protocol);

  protected:
    // Gets the file descriptor. Generally speaking, native handle getters should
    // not be `const`. This function exists for convenience for derived classes.
    int
    do_get_fd() const noexcept
      { return this->m_fd.get();  }

    // Marks this socket closed after `shutdown()`.
    void
    do_abstract_socket_set_closed() noexcept
      { this->m_state.store(socket_closed);  }

    // Gets the network driver instance inside the callbacks hereafter. If this
    // function is called elsewhere, the behavior is undefined.
    Network_Driver&
    do_abstract_socket_lock_driver(recursive_mutex::unique_lock& lock) const noexcept;

    linear_buffer&
    do_abstract_socket_lock_read_queue(recursive_mutex::unique_lock& lock) noexcept;

    linear_buffer&
    do_abstract_socket_lock_write_queue(recursive_mutex::unique_lock& lock) noexcept;

    bool
    do_abstract_socket_change_state(Socket_State from, Socket_State to) noexcept;

    // This callback is invoked by the network thread when the socket has
    // been closed, and is intended to be overriden by derived classes.
    // `errno` is zero for normal closure, or an error number in the case of
    // an error.
    virtual
    void
    do_abstract_socket_on_closed() = 0;

    // This callback is invoked by the network thread when incoming data are
    // available, and is intended to be overriden by derived classes.
    virtual
    void
    do_abstract_socket_on_readable() = 0;

    // This callback is invoked by the network thread when incoming
    // out-of-band data are available, and is intended to be overriden by
    // derived classes.
    virtual
    void
    do_abstract_socket_on_oob_readable() = 0;

    // This callback is invoked by the network thread when outgoing data are
    // possible, and is intended to be overriden by derived classes.
    virtual
    void
    do_abstract_socket_on_writable() = 0;

  public:
    Abstract_Socket(const Abstract_Socket&) = delete;
    Abstract_Socket& operator=(const Abstract_Socket&) & = delete;
    virtual ~Abstract_Socket();

    // Gets the file descriptor.
    int
    fd() noexcept
      { return this->m_fd.get();  }

    // Gets the socket state.
    Socket_State
    socket_state() const noexcept
      { return this->m_state.load();  }

    // Gets the local or bound address of this socket. In case of errors,
    // `ipv6_invalid` is returned. The result is cached and will not
    // reflect changes that other APIs may have made.
    ROCKET_PURE
    const IPv6_Address&
    local_address() const noexcept;

    // Checks whether the write (send) queue is empty.
    bool
    idle() const noexcept;

    // Initiates a connection for a stream socket, or sets the peer address
    // for a datagram socket.
    void
    connect(const IPv6_Address& addr);

    // Shuts the socket down without sending any protocol-specific closure
    // notifications.
    // This function is thread-safe.
    bool
    quick_close() noexcept;
  };

}  // namespace poseidon
#endif
