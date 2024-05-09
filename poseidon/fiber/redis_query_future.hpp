// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_REDIS_QUERY_FUTURE_
#define POSEIDON_FIBER_REDIS_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../redis/redis_value.hpp"
namespace poseidon {

class Redis_Query_Future
  :
    public Abstract_Future
  {
  public:
    // This is actually an input/output type.
    struct Result
      {
        vector<cow_string> cmd;  // input
        Redis_Value reply;
      };

  private:
    Redis_Connector* m_ctr;
    uniptr<Redis_Connection> m_conn;
    Result m_res;

  public:
    // Constructs a future for a single Redis command. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Executor`.
    // This future will become ready once the query is complete.
    Redis_Query_Future(Redis_Connector& connector, vector<cow_string> cmd);

  private:
    virtual
    void
    do_on_abstract_future_execute() override;

    virtual
    void
    do_on_abstract_future_finalize() noexcept override;

  public:
    Redis_Query_Future(const Redis_Query_Future&) = delete;
    Redis_Query_Future& operator=(const Redis_Query_Future&) & = delete;
    virtual ~Redis_Query_Future();

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
