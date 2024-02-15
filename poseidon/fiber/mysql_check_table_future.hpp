// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_MYSQL_CHECK_TABLE_FUTURE_
#define POSEIDON_FIBER_MYSQL_CHECK_TABLE_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_async_task.hpp"
#include "../mysql/mysql_table_structure.hpp"
namespace poseidon {

class MySQL_Check_Table_Future
  :
    public Abstract_Future,
    public Abstract_Async_Task
  {
  public:
    // This is actually an input/output type.
    struct Result
      {
        MySQL_Table_Structure table;  // input
        uint32_t warning_count;
        bool altered;
      };

  private:
    MySQL_Connector* m_connector;
    Result m_res;

  public:
    // Constructs a future for a table check request. This object also functions
    // as an asynchronous task, which can be enqueued into an `Async_Task_Executor`.
    // This future will become ready once the check is complete.
    MySQL_Check_Table_Future(MySQL_Connector& connector, MySQL_Table_Structure table);

  private:
    // Performs the table check.
    virtual
    void
    do_on_abstract_future_execute() override;

    virtual
    void
    do_on_abstract_async_task_execute() override;

  public:
    MySQL_Check_Table_Future(const MySQL_Check_Table_Future&) = delete;
    MySQL_Check_Table_Future& operator=(const MySQL_Check_Table_Future&) & = delete;
    virtual ~MySQL_Check_Table_Future();

    bool
    has_result() const noexcept
      { return this->successful();  }

    const Result&
    result() const
      { return this->do_check_success(this->m_res);  }

    Result&
    mut_result()
      { return this->do_check_success(this->m_res);  }
  };

}  // namespace poseidon
#endif
