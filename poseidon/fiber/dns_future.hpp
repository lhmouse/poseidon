// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_DNS_FUTURE_
#define POSEIDON_FIBER_DNS_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_async_task.hpp"
#include "../socket/socket_address.hpp"
namespace poseidon {

class DNS_Future
  :
    public Abstract_Future,
    public Abstract_Async_Task
  {
  public:
    // This is actually an input/output type.
    struct Result
      {
        cow_string host;
        uint16_t port;
        cow_vector<Socket_Address> addrs;
      };

  private:
    Result m_res;

  public:
    // Constructs a DNS result future. This object also functions as an
    // asynchronous task, which can be enqueued into an `Async_Task_Executor`.
    // This future will become ready once the DNS query is complete.
    explicit DNS_Future(cow_stringR host, uint16_t port = 0);

  private:
    // Performs DNS lookup.
    virtual
    void
    do_on_abstract_future_execute() override;

    virtual
    void
    do_on_abstract_async_task_execute() override;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(DNS_Future);

    bool
    has_result() const noexcept
      { return this->successful();  }

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
