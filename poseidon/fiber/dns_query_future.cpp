// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "dns_query_future.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

DNS_Query_Future::
DNS_Query_Future(cow_stringR host, uint16_t port)
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
do_on_abstract_future_execute()
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

    // Copy records into `m_res`.
    const auto guard = ::rocket::make_unique_handle(res, ::freeaddrinfo);
    Socket_Address saddr;

    for(res = guard; res;  res = res->ai_next)
      if(res->ai_family == AF_INET) {
        // IPv4
        ::memcpy(saddr.mut_data(), ipv4_unspecified.data(), 12);
        ::memcpy(saddr.mut_data() + 12, &(((::sockaddr_in*) res->ai_addr)->sin_addr), 4);
        saddr.set_port(this->m_res.port);
        POSEIDON_LOG_DEBUG(("Using IPv4: `$1` => `$2`"), m_res.host, saddr);

        if(::rocket::none_of(this->m_res.addrs, [&](const auto& r) { return r == saddr;  }))
          this->m_res.addrs.push_back(saddr);
      }
      else if(res->ai_family == AF_INET6) {
        // IPv6
        ::memcpy(saddr.mut_data(), &(((::sockaddr_in6*) res->ai_addr)->sin6_addr), 16);
        saddr.set_port(this->m_res.port);
        POSEIDON_LOG_DEBUG(("Using IPv6: `$1` => `$2`"), this->m_res.host, saddr);

        if(::rocket::none_of(this->m_res.addrs, [&](const auto& r) { return r == saddr;  }))
          this->m_res.addrs.push_back(saddr);
      }
  }

void
DNS_Query_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
