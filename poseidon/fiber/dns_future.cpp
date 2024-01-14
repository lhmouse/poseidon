// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "dns_future.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

DNS_Future::
DNS_Future(cow_stringR host, uint16_t port)
  {
    this->m_result.host = host;
    this->m_result.port = port;
  }

DNS_Future::
~DNS_Future()
  {
  }

void
DNS_Future::
do_on_abstract_future_execute()
  {
    // Perform DNS query. This will block the worker thread.
    ::addrinfo hints = { };
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
    ::addrinfo* res;
    int err = ::getaddrinfo(this->m_result.host.safe_c_str(), nullptr, &hints, &res);
    if(err != 0)
      POSEIDON_THROW((
          "Could not resolve host `$1`",
          "[`getaddrinfo()` failed: $2]"),
          this->m_result.host, ::gai_strerror(err));

    // Copy records into `m_result`.
    const ::rocket::unique_ptr<::addrinfo, void (::addrinfo*)> guard(res, ::freeaddrinfo);

    for(res = guard; res;  res = res->ai_next)
      if(res->ai_family == AF_INET) {
        // IPv4
        Socket_Address addr;
        ::memcpy(addr.mut_data(), ipv4_unspecified.data(), 12);
        ::memcpy(addr.mut_data() + 12, &(((::sockaddr_in*) res->ai_addr)->sin_addr), 4);
        addr.set_port(this->m_result.port);

        // Ignore duplicate records.
        if(find(this->m_result.addrs, addr) == nullptr)
          this->m_result.addrs.push_back(addr);
      }
      else if(res->ai_family == AF_INET6) {
        // IPv6
        Socket_Address addr;
        ::memcpy(addr.mut_data(), &(((::sockaddr_in6*) res->ai_addr)->sin6_addr), 16);
        addr.set_port(this->m_result.port);

        // Ignore duplicate records.
        if(find(this->m_result.addrs, addr) == nullptr)
          this->m_result.addrs.push_back(addr);
      }
  }

void
DNS_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
