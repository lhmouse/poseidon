// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_REDIS_QUERY_FUTURE_
#define POSEIDON_FIBER_REDIS_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_task.hpp"
#include "../redis/redis_value.hpp"
namespace poseidon {

class Redis_Query_Future
  :
    public Abstract_Future,
    public Abstract_Task
  {
  private:
    Redis_Connector* m_ctr;
    uniptr<Redis_Connection> m_conn;
    cow_vector<cow_string> m_cmd;
    Redis_Value m_res;

  public:
    // Constructs a future for a single Redis command. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Executor`.
    // This future will become ready once the query is complete.
    Redis_Query_Future(Redis_Connector& connector, uniptr<Redis_Connection>&& conn_opt,
                       const cow_vector<cow_string>& cmd);

    Redis_Query_Future(Redis_Connector& connector, const cow_vector<cow_string>& cmd);

  private:
    virtual
    void
    do_on_abstract_future_initialize() override;

    virtual
    void
    do_on_abstract_future_finalize() override;

    virtual
    void
    do_on_abstract_task_execute() override;

  public:
    Redis_Query_Future(const Redis_Query_Future&) = delete;
    Redis_Query_Future& operator=(const Redis_Query_Future&) & = delete;
    virtual ~Redis_Query_Future();

    // Gets the command to execute. This field is set by the constructor.
    const cow_vector<cow_string>&
    cmd() const noexcept
      { return this->m_cmd;  }

    // Gets the result after the operation has completed successfully. If
    // `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    const Redis_Value&
    result() const
      {
        this->check_success();
        return this->m_res;
      }
  };

}  // namespace poseidon
#endif
