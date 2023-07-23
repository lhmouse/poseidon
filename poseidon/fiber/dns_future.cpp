// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "dns_future.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

DNS_Future::
DNS_Future(cow_stringR host)
  {
    this->m_result.host = host;
  }

DNS_Future::
~DNS_Future()
  {
  }

void
DNS_Future::
do_abstract_task_on_execute()
  try {
    // Perform DNS query. This will block the worker thread.
    ::addrinfo hints = { };
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
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
        ::memcpy(addr.mut_data(), "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF", 12);
        ::memcpy(addr.mut_data() + 12, &(((::sockaddr_in*) res->ai_addr)->sin_addr), 4);

        // Ignore duplicate records.
        if(find(this->m_result.addrs, addr) == nullptr)
          this->m_result.addrs.push_back(addr);
      }
      else if(res->ai_family == AF_INET6) {
        // IPv6
        Socket_Address addr;
        ::memcpy(addr.mut_data(), &(((::sockaddr_in6*) res->ai_addr)->sin6_addr), 16);

        // Ignore duplicate records.
        if(find(this->m_result.addrs, addr) == nullptr)
          this->m_result.addrs.push_back(addr);
      }

    // Set the future as a success.
    this->do_set_ready(nullptr);
  }
  catch(exception& stdex) {
    POSEIDON_LOG_WARN(("Could not resolve host `$1`: $2"), this->m_result.host, stdex);

    // Set the future as a failure.
    this->do_set_ready(::std::current_exception());
  }

}  // namespace poseidon
