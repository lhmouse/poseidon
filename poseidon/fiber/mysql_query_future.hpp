// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_MYSQL_QUERY_FUTURE_
#define POSEIDON_FIBER_MYSQL_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../mysql/mysql_value.hpp"
namespace poseidon {

class MySQL_Query_Future
  :
    public Abstract_Future
  {
  public:
    // This is actually an input/output type.
    struct Result
      {
        cow_string stmt;  // input
        vector<MySQL_Value> stmt_args;  // input
        uint32_t warning_count;
        uint64_t affected_rows;
        uint64_t insert_id;
        vector<cow_string> result_fields;
        vector<vector<MySQL_Value>> result_rows;
      };

  private:
    MySQL_Connector* m_ctr;
    Result m_res;

  public:
    // Constructs a future for a single MySQL statement. This object also functions
    // as an asynchronous task, which can be enqueued into an `Async_Task_Executor`.
    // This future will become ready once the query is complete.
    MySQL_Query_Future(MySQL_Connector& connector, cow_stringR stmt);

    MySQL_Query_Future(MySQL_Connector& connector, cow_stringR stmt, vector<MySQL_Value> stmt_args);

  private:
    virtual
    void
    do_on_abstract_future_execute() override;

  public:
    MySQL_Query_Future(const MySQL_Query_Future&) = delete;
    MySQL_Query_Future& operator=(const MySQL_Query_Future&) & = delete;
    virtual ~MySQL_Query_Future();

    // Gets the result if `successful()` yields `true`. If `successful()` yields
    // `false`, an exception is thrown, and there is no effect.
    const Result&
    result() const
      { return this->do_check_success(this->m_res);  }

    Result&
    mut_result()
      { return this->do_check_success(this->m_res);  }
  };

}  // namespace poseidon
#endif
