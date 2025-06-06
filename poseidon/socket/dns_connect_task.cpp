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
DNS_Connect_Task(Network_Driver& driver, const shptr<Abstract_Socket>& socket, const cow_string& host, uint16_t port)
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
    opt<IPv6_Address> dns_result;
    shptr<Abstract_Socket> socket;

    try {
      // Perform DNS query. This will block the worker thread.
      ::addrinfo hints = { };
      hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
      ::addrinfo* res;
      int err = ::getaddrinfo(this->m_host.safe_c_str(), nullptr, &hints, &res);
      if(err != 0)
        POSEIDON_THROW((
            "Could not resolve host name `$1`",
            "[`getaddrinfo()` failed: $2]"),
            this->m_host, ::gai_strerror(err));

      const auto guard = make_unique_handle(res, ::freeaddrinfo);

      // Iterate over the list and find a suitable address to connect. IPv4
      // addresses are preferred to IPv6 ones, so this has to be done as two
      // passes.
      for(res = guard;  res && !dns_result;  res = res->ai_next)
        if(res->ai_family == AF_INET) {
          // IPv4
          auto& saddr = dns_result.value_or_emplace();
          ::memcpy(saddr.mut_data(), ipv4_unspecified.data(), 12);
          ::memcpy(saddr.mut_data() + 12, &(((::sockaddr_in*) res->ai_addr)->sin_addr), 4);
          saddr.set_port(this->m_port);
          POSEIDON_LOG_DEBUG(("Using IPv4: `$1` => `$2`"), this->m_host, saddr);
        }

      for(res = guard;  res && !dns_result;  res = res->ai_next)
        if(res->ai_family == AF_INET6) {
          // IPv6
          auto& saddr = dns_result.value_or_emplace();
          ::memcpy(saddr.mut_data(), &(((::sockaddr_in6*) res->ai_addr)->sin6_addr), 16);
          saddr.set_port(this->m_port);
          POSEIDON_LOG_DEBUG(("Using IPv6: `$1` => `$2`"), this->m_host, saddr);
        }
    }
    catch(exception& stdex) {
      // Shut the connection down later.
      POSEIDON_LOG_ERROR(("Could not resolve host name: $1"), stdex);
      dns_result.reset();
    }

    socket = this->m_wsock.lock();
    if(!socket)
      return;

    try {
      if(dns_result)
        socket->connect(*dns_result);
      else
        socket->quick_close();
    }
    catch(exception& stdex) {
      POSEIDON_LOG_ERROR(("Could not initiate connection: $1"), stdex);
      socket->quick_close();
    }

    // Insert the socket. Even in case of a failure, a closure notification
    // shall be delivered.
    this->m_driver->insert(socket);
  }

}  // namespace poseidon
