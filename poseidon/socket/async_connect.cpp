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
Async_Connect(Network_Driver& driver, shptrR<Abstract_Socket> socket, cow_stringR host, uint16_t port)
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
do_abstract_task_on_execute()
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
            "Could not resolve host `$1`",
            "[`getaddrinfo()` failed: $2]"),
            this->m_host, ::gai_strerror(err));

      // Iterate over the list and find a suitable address to connect. IPv4
      // addresses are preferred to IPv6 ones, so this has to be done as two
      // passes.
      const ::rocket::unique_ptr<::addrinfo, void (::addrinfo*)> guard(res, ::freeaddrinfo);

      for(res = guard;  res;  res = res->ai_next)
        if(res->ai_family == AF_INET) {
          // IPv4
          Socket_Address addr;
          ::memcpy(addr.mut_data(), ipv4_unspecified.data(), 12);
          ::memcpy(addr.mut_data() + 12, &(((::sockaddr_in*) res->ai_addr)->sin_addr), 4);
          addr.set_port(this->m_port);

          socket->connect(addr);
          this->m_driver->insert(socket);
          POSEIDON_LOG_DEBUG(("Initiating new connection to `$1`"), addr);
          return;
        }

      for(res = guard;  res;  res = res->ai_next)
        if(res->ai_family == AF_INET6) {
          // IPv6
          Socket_Address addr;
          ::memcpy(addr.mut_data(), &(((::sockaddr_in6*) res->ai_addr)->sin6_addr), 16);
          addr.set_port(this->m_port);

          socket->connect(addr);
          this->m_driver->insert(socket);
          POSEIDON_LOG_DEBUG(("Initiating new connection to `$1`"), addr);
          return;
        }

      // There is no suitable address, so fail.
      POSEIDON_THROW((
          "No suitable address for host `$3`",
          "[socket `$1` (class `$2`)]"),
          socket, typeid(*socket), this->m_host);
    }
    catch(exception& stdex) {
      // Connection cannot be initiated, so close the socket.
      POSEIDON_LOG_ERROR(("Could not resolve host `$1`: $2"), this->m_host, stdex);
      socket->quick_close();
      this->m_driver->insert(socket);
    }
  }

}  // namespace poseidon
