// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "dns_connect_task.hpp"
#include "abstract_socket.hpp"
#include "../static/network_scheduler.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

DNS_Connect_Task::
DNS_Connect_Task(Network_Scheduler& scheduler, const shptr<Abstract_Socket>& socket,
                 const cow_string& host, uint16_t port)
  {
    if(!socket)
      POSEIDON_THROW(("Null socket pointer not valid"));

    this->m_scheduler = &scheduler;
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
    shptr<Abstract_Socket> socket;
    bool success = false;

    try {
      // Perform DNS query. This will block the worker thread.
      ::addrinfo* res = nullptr;
      ::rocket::unique_ptr<::addrinfo, vfn<::addrinfo*>> guard(res, ::freeaddrinfo);

      ::addrinfo hints = { };
      hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
      int err = ::getaddrinfo(this->m_host.safe_c_str(), nullptr, &hints, &res);
      if(err == 0)
        guard.reset(res);
      else
        POSEIDON_THROW((
            "Could not resolve host `$1`",
            "[`getaddrinfo()` failed: $2]"),
            this->m_host, ::gai_strerror(err));

      socket = this->m_wsock.lock();
      if(!socket)
        return;

      for(res = guard;  !success && res;  res = res->ai_next)
        if(res->ai_family == AF_INET) {
          // IPv4
          auto sa = reinterpret_cast<::sockaddr_in*>(res->ai_addr);
          ::sockaddr_in6 v6addr = { };
          v6addr.sin6_family = AF_INET6;
          ::memcpy(v6addr.sin6_addr.s6_addr, ipv4_unspecified.data(), 12);
          ::memcpy(v6addr.sin6_addr.s6_addr + 12, &(sa->sin_addr), 4);
          v6addr.sin6_port = ::htons(this->m_port);
          success = (::connect(socket->fd(),
                               reinterpret_cast<const ::sockaddr*>(&v6addr),
                               sizeof(::sockaddr_in6)) == 0)
                    || (errno == EINPROGRESS);
        }
        else if(res->ai_family == AF_INET6) {
          // IPv6
          auto sa = reinterpret_cast<::sockaddr_in6*>(res->ai_addr);
          sa->sin6_port = ::htons(this->m_port);
          success = (::connect(socket->fd(),
                               reinterpret_cast<const ::sockaddr*>(sa),
                               sizeof(::sockaddr_in6)) == 0)
                    || (errno == EINPROGRESS);
        }

      // Insert the socket, only after `connect()` has been called. Even in
      // the case of a failure, a closure notification shall be delivered.
      if(success)
        this->m_scheduler->insert_weak(socket);
      else
        POSEIDON_LOG_WARN(("DNS lookup failed for `$1`"), this->m_host);
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR(("DNS lookup error: $1"), stdex);
      success = false;
    }

    if(!socket) {
      socket = this->m_wsock.lock();
      if(!socket)
        return;
    }

    if(!success)
      socket->quick_shut_down();
  }

}  // namespace poseidon
