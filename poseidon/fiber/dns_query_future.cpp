// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "dns_query_future.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

DNS_Query_Future::
DNS_Query_Future(const cow_string& host, uint16_t port)
  {
    this->m_res.host = host;
    this->m_res.port = port;
  }

DNS_Query_Future::
~DNS_Query_Future()
  {
  }

void
DNS_Query_Future::
do_on_abstract_future_initialize()
  {
    // Perform DNS query. This will block the worker thread.
    ::addrinfo hints = { };
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
    ::addrinfo* res;
    int err = ::getaddrinfo(this->m_res.host.safe_c_str(), nullptr, &hints, &res);
    if(err != 0)
      POSEIDON_THROW((
          "Could not resolve host `$1`",
          "[`getaddrinfo()` failed: $2]"),
          this->m_res.host, ::gai_strerror(err));

    const auto guard = make_unique_handle(res, ::freeaddrinfo);

    for(res = guard;  res;  res = res->ai_next) {
      IPv6_Address addr;
      if(res->ai_family == AF_INET) {
        // IPv4
        auto sa = reinterpret_cast<::sockaddr_in*>(res->ai_addr);
        ::memcpy(addr.mut_data(), ipv4_unspecified.data(), 12);
        ::memcpy(addr.mut_data() + 12, &(sa->sin_addr), 4);
        addr.set_port(this->m_res.port);
      }
      else if(res->ai_family == AF_INET6) {
        // IPv6
        auto sa = reinterpret_cast<::sockaddr_in6*>(res->ai_addr);
        ::memcpy(addr.mut_data(), &(sa->sin6_addr), 16);
        addr.set_port(this->m_res.port);
      }
      else
        continue;

      if(is_any_of(addr, this->m_res.addrs))
        continue;

      POSEIDON_LOG_DEBUG(("DNS lookup: `$1` => `$2`"), this->m_res.host, addr);
      this->m_res.addrs.push_back(addr);
    }
  }

void
DNS_Query_Future::
do_on_abstract_task_execute()
  {
    this->do_abstract_future_initialize_once();
  }

}  // namespace poseidon
