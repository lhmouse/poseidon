// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_MYSQL_QUERY_FUTURE_
#define POSEIDON_FIBER_MYSQL_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_task.hpp"
#include "../mysql/mysql_value.hpp"
namespace poseidon {

class MySQL_Query_Future
  :
    public Abstract_Future,
    public Abstract_Task
  {
  public:
    // This is actually an input/output type.
    struct Result
      {
        cow_string stmt;  // input
        cow_vector<MySQL_Value> stmt_args;  // input
        uint32_t warning_count;
        uint64_t affected_rows;
        uint64_t insert_id;
        cow_vector<cow_string> result_fields;
        cow_vector<cow_vector<MySQL_Value>> result_rows;
      };

  private:
    MySQL_Connector* m_ctr;
    uniptr<MySQL_Connection> m_conn;
    Result m_res;

  public:
    // Constructs a future for a single MySQL statement. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Executor`.
    // This future will become ready once the query is complete.
    MySQL_Query_Future(MySQL_Connector& connector, uniptr<MySQL_Connection>&& conn_opt,
                       const cow_string& stmt, const cow_vector<MySQL_Value>& stmt_args);

    MySQL_Query_Future(MySQL_Connector& connector, const cow_string& stmt,
                       const cow_vector<MySQL_Value>& stmt_args);

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
    MySQL_Query_Future(const MySQL_Query_Future&) = delete;
    MySQL_Query_Future& operator=(const MySQL_Query_Future&) & = delete;
    virtual ~MySQL_Query_Future();

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
