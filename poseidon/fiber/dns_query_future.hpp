// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_DNS_QUERY_FUTURE_
#define POSEIDON_FIBER_DNS_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../socket/ipv6_address.hpp"
namespace poseidon {

class DNS_Query_Future
  :
    public Abstract_Future
  {
  public:
    // This is actually an input/output type.
    struct Result
      {
        cow_string host;
        uint16_t port;
        cow_vector<IPv6_Address> addrs;
      };

  private:
    Result m_res;

  public:
    // Constructs a DNS query future. This object also functions as an asynchronous
    // task, which can be enqueued into an `Task_Executor`. This future will
    // become ready once the DNS query is complete.
    DNS_Query_Future(const cow_string& host, uint16_t port);

  private:
    virtual
    void
    do_on_abstract_future_execute() override;

  public:
    DNS_Query_Future(const DNS_Query_Future&) = delete;
    DNS_Query_Future& operator=(const DNS_Query_Future&) & = delete;
    virtual ~DNS_Query_Future();

    // Gets the result if `successful()` yields `true`. If `successful()` yields
    // `false`, an exception is thrown, and there is no effect.
    const Result&
    result() const
      {
        this->check_success();
        return this->m_res;
      }

    Result&
    mut_result()
      {
        this->check_success();
        return this->m_res;
      }
  };

}  // namespace poseidon
#endif
