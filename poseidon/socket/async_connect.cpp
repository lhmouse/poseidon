// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "async_connect.hpp"
#include "abstract_socket.hpp"
#include "../static/network_driver.hpp"
#include "../utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
namespace poseidon {

Async_Connect::
Async_Connect(Network_Driver& driver, shR<Abstract_Socket> socket, cow_stringR host, uint16_t port)
  {
    if(!socket)
      POSEIDON_THROW(("Null socket pointer not valid"));

    this->m_driver = &driver;
    this->m_wsock = socket;
    this->m_host = host;
    this->m_port = port;
  }

Async_Connect::
~Async_Connect()
  {
  }

void
Async_Connect::
do_on_abstract_async_task_execute()
  {
    const auto socket = this->m_wsock.lock();
    if(!socket)
      return;

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

      const ::rocket::unique_ptr<::addrinfo, void (::addrinfo*)> guard(res, ::freeaddrinfo);
      opt<Socket_Address> dns_result;

      // Iterate over the list and find a suitable address to connect. IPv4
      // addresses are preferred to IPv6 ones, so this has to be done as two
      // passes.
      for(res = guard;  res && !dns_result;  res = res->ai_next)
        if(res->ai_family == AF_INET) {
          // IPv4
          auto& addr = dns_result.emplace();
          ::memcpy(addr.mut_data(), ipv4_unspecified.data(), 12);
          ::memcpy(addr.mut_data() + 12, &(((::sockaddr_in*) res->ai_addr)->sin_addr), 4);
          addr.set_port(this->m_port);
          POSEIDON_LOG_DEBUG(("Using IPv4: `$1` => `$2`"), this->m_host, addr);
        }

      for(res = guard;  res && !dns_result;  res = res->ai_next)
        if(res->ai_family == AF_INET6) {
          // IPv6
          auto& addr = dns_result.emplace();
          ::memcpy(addr.mut_data(), &(((::sockaddr_in6*) res->ai_addr)->sin6_addr), 16);
          addr.set_port(this->m_port);
          POSEIDON_LOG_DEBUG(("Using IPv6: `$1` => `$2`"), this->m_host, addr);
        }


      if(dns_result)
        socket->connect(*dns_result);
      else
        socket->quick_close();

      // Insert the socket. Even in case of a failure, a closure notification
      // shall be delivered.
      this->m_driver->insert(socket);
    }
    catch(exception& stdex) {
      // Shut the connection down later.
      POSEIDON_LOG_ERROR(("DNS lookup error: $1"), stdex);
      socket->quick_close();
    }
  }

}  // namespace poseidon
