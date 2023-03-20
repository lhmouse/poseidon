// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "dns_future.hpp"
#include "../utils.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

DNS_Future::
DNS_Future(stringR host)
  : m_host(host)
  {
  }

DNS_Future::
~DNS_Future()
  {
  }

void
DNS_Future::
do_abstract_task_on_execute()
  try {
    // Warning: This is in the worker thread. Any operation that changes data
    // members of `*this` shall be put into `do_on_future_ready()`, where `*this`
    // will have been locked.
    ::addrinfo* res;
    int r = ::getaddrinfo(this->m_host.safe_c_str(), nullptr, nullptr, &res);
    if(r != 0)
      POSEIDON_THROW((
          "Could not perform DNS query for host `$1`",
          "[`getaddrinfo()` failed: $2]"),
          this->m_host, ::gai_strerror(r));

    // Copy records into `m_result`. This requires locking `*this` so do it in
    // `do_on_future_ready)(`.
    ::rocket::unique_ptr<::addrinfo, void (::addrinfo*)> guard(res, ::freeaddrinfo);
    this->do_try_set_ready(guard);
    POSEIDON_LOG_DEBUG(("DNS query success: host = $1"), this->m_host);
  }
  catch(exception& stdex) {
    // Ensure an exception is always set, even after the first call to
    // `do_try_set_ready()` has failed.
    POSEIDON_LOG_DEBUG(("DNS query error: host = $1, stdex = $2"), this->m_host, stdex);
    this->do_try_set_ready(nullptr);
  }

void
DNS_Future::
do_on_future_ready(void* param)
  {
    if(!param) {
      // Set an exception.
      this->m_except = ::std::current_exception();
      return;
    }

    this->m_result.clear();
    this->m_except = nullptr;

    // Copy records.
    ::addrinfo* res = (::addrinfo*) param;
    while(res) {
      optional <Socket_Address> addr;

      if(res->ai_family == AF_INET) {
        // IPv4
        char bytes[16] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF";
        ::memcpy(bytes + 12, &(((::sockaddr_in*) res->ai_addr)->sin_addr), 4);
        ::memcpy(addr.emplace().mut_data(), bytes, 16);
      }
      else if(res->ai_family == AF_INET6) {
        // IPv6
        ::memcpy(addr.emplace().mut_data(), &(((::sockaddr_in6*) res->ai_addr)->sin6_addr), 16);
      }

      if(addr) {
        POSEIDON_LOG_DEBUG(("DNS record: host = $1, addr = $2"), this->m_host, *addr);

        // Ignore duplicate records.
        if(find(this->m_result, *addr) == nullptr)
          this->m_result.push_back(*addr);
      }

      // Go to the next record.
      res = res->ai_next;
    }
  }

}  // namespace poseidon
