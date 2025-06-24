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
  private:
    MySQL_Connector* m_ctr;
    uniptr<MySQL_Connection> m_conn;
    cow_string m_stmt;
    cow_vector<MySQL_Value> m_stmt_args;
    uint32_t m_warning_count;
    uint64_t m_affected_rows;
    uint64_t m_insert_id;
    cow_vector<cow_string> m_result_fields;
    cow_vector<cow_vector<MySQL_Value>> m_result_rows;

  public:
    // Constructs a future for a single MySQL statement. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Scheduler`.
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

    // Gets the statement to execute. This field is set by the constructor.
    const cow_string&
    stmt() const noexcept
      { return this->m_stmt;  }

    // Gets the arguments for the statement to execute. This field is set by
    // the constructor.
    const cow_vector<MySQL_Value>&
    stmt_args() const noexcept
      { return this->m_stmt_args;  }

    // Gets the number of warnings of the last operation. This can indicate
    // whether an INSERT IGNORE operation fails due to a key conflict. If
    // `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    uint32_t
    warning_count() const
      {
        this->check_success();
        return this->m_warning_count;
      }

    // Gets the number of rows that have been affected by the last operation.
    // The value is undefined for other operations. If `successful()` yields
    // `false`, an exception is thrown, and there is no effect.
    uint64_t
    affected_rows() const noexcept
      {
        this->check_success();
        return this->m_affected_rows;
      }

    // Gets the auto-increment ID of an INSERT or REPLACE operation, after it
    // has completed successfully. The value is undefined for other operations.
    // If `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    uint64_t
    insert_id() const
      {
        this->check_success();
        return this->m_insert_id;
      }

    // Gets all result fields after the operation has completed successfully. If
    // `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    const cow_vector<cow_string>&
    result_fields() const
      {
        this->check_success();
        return this->m_result_fields;
      }

    // Gets all result rows after the operation has completed successfully. If
    // `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    const cow_vector<cow_vector<MySQL_Value>>&
    result_rows() const
      {
        this->check_success();
        return this->m_result_rows;
      }
  };

}  // namespace poseidon
#endif
