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
  : public Abstract_Future,
    public Abstract_Async_Task
  {
  private:
    string m_host;
    cow_vector<Socket_Address> m_result;
    exception_ptr m_except;

  public:
    explicit
    DNS_Future(stringR host);

  private:
    // Performs DNS lookup.
    virtual
    void
    do_abstract_task_on_execute() override;

    // Sets the result.
    virtual
    void
    do_on_future_ready(void* param) override;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(DNS_Future);

    // Gets the result.
    const cow_vector<Socket_Address>&
    result() const
      {
        if(!this->ready())
          ::rocket::sprintf_and_throw<::std::runtime_error>(
              "DNS_Future: DNS query in progress");

        if(this->m_except)
          ::std::rethrow_exception(this->m_except);

        return this->m_result;
      }
  };

}  // namespace poseidon
#endif
