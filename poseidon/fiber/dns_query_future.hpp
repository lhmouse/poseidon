// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_DNS_QUERY_FUTURE_
#define POSEIDON_FIBER_DNS_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_task.hpp"
#include "../socket/ipv6_address.hpp"
namespace poseidon {

class DNS_Query_Future
  :
    public Abstract_Future,
    public Abstract_Task
  {
  private:
    cow_string m_host;
    uint16_t m_port;
    cow_vector<IPv6_Address> m_res;

  public:
    // Constructs a DNS query future. This object also functions as an
    // asynchronous task which can be enqueued into an `Task_Scheduler`. This
    // future will become ready once the DNS query is complete.
    DNS_Query_Future(const cow_string& host, uint16_t port);

  private:
    virtual
    void
    do_on_abstract_future_initialize()
      override;

    virtual
    void
    do_on_abstract_task_execute()
      override;

  public:
    DNS_Query_Future(const DNS_Query_Future&) = delete;
    DNS_Query_Future& operator=(const DNS_Query_Future&) & = delete;
    virtual ~DNS_Query_Future();

    // Gets the hostname to look up. This field is set by the constructor.
    const cow_string&
    host()
      const noexcept
      { return this->m_host;  }

    // Gets the port to set in the result addresses. This field is set by the
    // constructor.
    uint16_t
    port()
      const noexcept
      { return this->m_port;  }

    // Gets the result after the operation has completed successfully. If
    // `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    const cow_vector<IPv6_Address>&
    result()
      const
      {
        this->check_success();
        return this->m_res;
      }
  };

}  // namespace poseidon
#endif
