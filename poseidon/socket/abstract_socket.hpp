// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_ABSTRACT_SOCKET_
#define POSEIDON_SOCKET_ABSTRACT_SOCKET_

#include "../fwd.hpp"
#include "socket_address.hpp"
namespace poseidon {

class Abstract_Socket
  {
  private:
    friend class Network_Driver;

    unique_posix_fd m_fd;
    atomic_relaxed<Socket_State> m_state;
    mutable atomic_relaxed<bool> m_sockname_ready;
    mutable Socket_Address m_sockname;

    mutable recursive_mutex m_io_mutex;
    Network_Driver* m_io_driver;
    bool m_io_throttled = false;
    linear_buffer m_io_read_queue;
    linear_buffer m_io_write_queue;

  protected:
    // Takes ownership of an existent IPv6 socket.
    explicit
    Abstract_Socket(unique_posix_fd&& fd);

    // Creates a new non-blocking IPv6 socket.
    explicit
    Abstract_Socket(int type, int protocol);

  protected:
    // Gets the file descriptor.
    // Generally speaking, native handle getters should not be `const`. This
    // function exists for convenience for derived classes.
    int
    do_get_fd() const noexcept
      { return this->m_fd.get();  }

    // Gets the network driver instance inside the callbacks hereafter. If this
    // function is called elsewhere, the behavior is undefined.
    Network_Driver&
    do_abstract_socket_lock_driver(recursive_mutex::unique_lock& lock) const noexcept
      {
        lock.lock(this->m_io_mutex);
        ROCKET_ASSERT(this->m_io_driver);
        return *(this->m_io_driver);
      }

    // Gets the read (receive) queue.
    linear_buffer&
    do_abstract_socket_lock_read_queue(recursive_mutex::unique_lock& lock) noexcept
      {
        lock.lock(this->m_io_mutex);
        return this->m_io_read_queue;
      }

    // Gets the write (send) queue.
    linear_buffer&
    do_abstract_socket_lock_write_queue(recursive_mutex::unique_lock& lock) noexcept
      {
        lock.lock(this->m_io_mutex);
        return this->m_io_write_queue;
      }

    // Sets the socket state.
    void
    do_abstract_socket_set_state(Socket_State to) noexcept
      {
        this->m_state.store(to);
      }

    bool
    do_abstract_socket_change_state(Socket_State from, Socket_State to) noexcept
      {
        Socket_State comp = from;
        bool eq = this->m_state.cmpxchg(comp, to);
        return eq;
      }

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
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Abstract_Socket);

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
    const Socket_Address&
    local_address() const noexcept;

    // Initiates a connection for a stream socket, or sets the peer address
    // for a datagram socket.
    void
    connect(const Socket_Address& addr);

    // Shuts the socket down without sending any protocol-specific closure
    // notifications.
    // This function is thread-safe.
    bool
    quick_close() noexcept;
  };

}  // namespace poseidon
#endif
