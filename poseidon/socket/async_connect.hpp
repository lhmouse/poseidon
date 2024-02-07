// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_ASYNC_CONNECT_
#define POSEIDON_SOCKET_ASYNC_CONNECT_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../base/abstract_async_task.hpp"
namespace poseidon {

class Async_Connect
  :
    public Abstract_Async_Task
  {
  private:
    Network_Driver* m_driver;
    wkptr<Abstract_Socket> m_wsock;
    cow_string m_host;
    uint16_t m_port;

  public:
    // Performs asynchronous DNS lookup. If at least one address is found,
    // `connect()` is called on `socket`. If both IPv4 and IPv6 addresses are
    // found, an IPv4 address is preferred to an IPv6 address. If no address can
    // be found, the socket is closed immediately.
    Async_Connect(Network_Driver& driver, shptrR<Abstract_Socket> socket, cow_stringR host, uint16_t port);

  protected:
    // This class implements `Abstract_Async_Task`.
    virtual
    void
    do_on_abstract_async_task_execute() override;

  public:
    Async_Connect(const Async_Connect&) = delete;
    Async_Connect& operator=(const Async_Connect&) & = delete;
    virtual ~Async_Connect();

    shptr<Abstract_Socket>
    socket_opt() const noexcept
      { return this->m_wsock.lock();  }

    cow_stringR
    host() const noexcept
      { return this->m_host;  }

    uint16_t
    port() const noexcept
      { return this->m_port;  }
  };

}  // namespace poseidon
#endif
