// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_DNS_CONNECT_TASK_
#define POSEIDON_SOCKET_DNS_CONNECT_TASK_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../base/abstract_task.hpp"
namespace poseidon {

class DNS_Connect_Task
  :
    public Abstract_Task
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
    DNS_Connect_Task(Network_Driver& driver, const shptr<Abstract_Socket>& socket,
                     const cow_string& host, uint16_t port);

  protected:
    // This class implements `Abstract_Task`.
    virtual
    void
    do_on_abstract_task_execute() override;

  public:
    DNS_Connect_Task(const DNS_Connect_Task&) = delete;
    DNS_Connect_Task& operator=(const DNS_Connect_Task&) & = delete;
    virtual ~DNS_Connect_Task();

    shptr<Abstract_Socket>
    socket_opt() const noexcept
      { return this->m_wsock.lock();  }

    const cow_string&
    host() const noexcept
      { return this->m_host;  }

    uint16_t
    port() const noexcept
      { return this->m_port;  }
  };

}  // namespace poseidon
#endif
