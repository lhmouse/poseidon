// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_ABSTRACT_SOCKET_
#define POSEIDON_SOCKET_ABSTRACT_SOCKET_

#include "../fwd.hpp"
#include "enums.hpp"
#include "ipv6_address.hpp"
namespace poseidon {

class Abstract_Socket
  {
  private:
    friend class Network_Scheduler;

    unique_posix_fd m_fd;
    atomic_relaxed<Socket_State> m_state;
    char m_padding_1;
    char m_padding_2;
    char m_padding_3;
    mutable atomic_acq_rel<bool> m_sockname_ready;
    mutable atomic_acq_rel<bool> m_peername_ready;

    mutable IPv6_Address m_sockname;
    mutable IPv6_Address m_peername;

    mutable recursive_mutex m_sched_mutex;
    Network_Scheduler* m_scheduler;
    linear_buffer m_sched_read_queue;
    linear_buffer m_sched_write_queue;
    bool m_sched_throttled = false;

  protected:
    // Take ownership of an existent IPv6 socket. [server-side constructor]
    explicit Abstract_Socket(unique_posix_fd&& fd);

    // Create a new non-blocking IPv6 socket. [client-side constructor]
    Abstract_Socket(int type, int protocol);

  protected:
    // Get the file descriptor. Generally speaking, native handle getters
    // should not be `const`. This function exists for convenience for derived
    // classes.
    int
    do_socket_fd()
      const noexcept
      { return this->m_fd.get();  }

    bool
    do_socket_test_change(Socket_State from, Socket_State to)
      noexcept
      {
        Socket_State old = this->m_state.load();
        return (old == from) && this->m_state.cmpxchg(old, to);
      }

    // Get the network scheduler instance inside the callbacks hereafter. If this
    // function is called elsewhere, the behavior is undefined.
    Network_Scheduler&
    do_abstract_socket_lock_scheduler(recursive_mutex::unique_lock& lock)
      const noexcept;

    linear_buffer&
    do_abstract_socket_lock_read_queue(recursive_mutex::unique_lock& lock)
      noexcept;

    linear_buffer&
    do_abstract_socket_lock_write_queue(recursive_mutex::unique_lock& lock)
      noexcept;

    // This callback is invoked by the network thread when incoming data are
    // available, and is intended to be overriden by derived classes.
    virtual
    void
    do_abstract_socket_on_readable()
      = 0;

    // This callback is invoked by the network thread when outgoing data are
    // possible, and is intended to be overriden by derived classes.
    virtual
    void
    do_abstract_socket_on_writeable()
      = 0;

    // This callback is invoked by the network thread when the socket has
    // been closed, and is intended to be overriden by derived classes.
    // `errno` is zero for normal closure, or an error number in the case of
    // an error.
    virtual
    void
    do_abstract_socket_on_closed()
      = 0;

  public:
    Abstract_Socket(const Abstract_Socket&) = delete;
    Abstract_Socket& operator=(const Abstract_Socket&) & = delete;
    virtual ~Abstract_Socket();

    // Get the file descriptor.
    int
    fd()
      noexcept
      { return this->m_fd.get();  }

    // Get the connection state.
    Socket_State
    socket_state()
      const noexcept
      { return this->m_state.load();  }

    // Get the local or bound address of this socket. In case of errors,
    // `ipv6_invalid` is returned. The result is cached and will not reflect
    // changes that other APIs may have made.
    ROCKET_PURE
    const IPv6_Address&
    local_address()
      const noexcept;

    // Get the remote or connected address of this socket. In case of errors,
    // `ipv6_invalid` is returned. The result is cached and will not reflect
    // changes that other APIs may have made.
    ROCKET_PURE
    const IPv6_Address&
    remote_address()
      const noexcept;

    // Shut the socket down without flushing unsent data nor sending any
    // protocol-specific notifications, useful for abandoning an abnormal
    // connection. Be advised that data loss or corruption may happen.
    // This function is thread-safe.
    bool
    quick_shut_down()
      noexcept;
  };

}  // namespace poseidon
#endif
