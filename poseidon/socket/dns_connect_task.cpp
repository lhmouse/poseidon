// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "dns_connect_task.hpp"
#include "abstract_socket.hpp"
#include "../static/network_driver.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

DNS_Connect_Task::
DNS_Connect_Task(Network_Driver& driver, const shptr<Abstract_Socket>& socket,
                 const cow_string& host, uint16_t port)
  {
    if(!socket)
      POSEIDON_THROW(("Null socket pointer not valid"));

    this->m_driver = &driver;
    this->m_wsock = socket;
    this->m_host = host;
    this->m_port = port;
  }

DNS_Connect_Task::
~DNS_Connect_Task()
  {
  }

void
DNS_Connect_Task::
do_on_abstract_task_execute()
  {
    // Perform DNS query. This will block the worker thread.
    ::addrinfo hints = { };
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
    ::addrinfo* res;
    int err = ::getaddrinfo(this->m_host.safe_c_str(), nullptr, &hints, &res);
    if(err != 0)
      POSEIDON_THROW((
          "Could not resolve host `$1`",
          "[`getaddrinfo()` failed: $2]"),
          this->m_host, ::gai_strerror(err));

    const auto guard = make_unique_handle(res, ::freeaddrinfo);
    ::sockaddr_in6 v4addr = { };
    ::sockaddr_in6 v6addr = { };

    for(res = guard;  res;  res = res->ai_next)
      if(res->ai_family == AF_INET) {
        // IPv4
        auto sa = reinterpret_cast<::sockaddr_in*>(res->ai_addr);
        v4addr.sin6_family = AF_INET6;
        ::memcpy(v4addr.sin6_addr.s6_addr, ipv4_unspecified.data(), 12);
        ::memcpy(v4addr.sin6_addr.s6_addr + 12, &(sa->sin_addr), 4);
        v4addr.sin6_port = ROCKET_HTOBE16(this->m_port);
      }
      else if(res->ai_family == AF_INET6) {
        // IPv6
        auto sa = reinterpret_cast<::sockaddr_in6*>(res->ai_addr);
        ::memcpy(&v6addr, sa, sizeof(v6addr));
        sa->sin6_port = ROCKET_HTOBE16(this->m_port);
      }

    auto socket = this->m_wsock.lock();
    if(!socket)
      return;

    try {
      // Prefer IPv4 to IPv6. TOOD: Make this configurable?
      if(v4addr.sin6_family != 0)
        v6addr = v4addr;

      if(v6addr.sin6_family == 0) {
        POSEIDON_LOG_WARN(("No viable address to host `$1`"), this->m_host);
        socket->close();
      }
      else if((::connect(socket->fd(), reinterpret_cast<sockaddr*>(&v6addr),
                         sizeof(v6addr)) != 0)
              && (errno != EINPROGRESS)
              && (errno != EAGAIN)) {
        POSEIDON_LOG_WARN(("Could not connect to `$1`: ${errno:full}"), this->m_host);
        socket->close();
      }

      // Insert the socket. Even in the case of a failure, a closure
      // notification shall be delivered.
      this->m_driver->insert(socket);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR(("Could not initiate TCP connection: $1"), stdex);
      socket->close();
    }
  }

}  // namespace poseidon
