// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_MYSQL_CHECK_TABLE_FUTURE_
#define POSEIDON_FIBER_MYSQL_CHECK_TABLE_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_task.hpp"
#include "../mysql/mysql_table_structure.hpp"
namespace poseidon {

class MySQL_Check_Table_Future
  :
    public Abstract_Future,
    public Abstract_Task
  {
  private:
    MySQL_Connector* m_ctr;
    uniptr<MySQL_Connection> m_conn;
    MySQL_Table_Structure m_table;
    bool m_altered = false;

  public:
    // Constructs a future for a table check request. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Scheduler`.
    // This future will become ready once the check is complete.
    MySQL_Check_Table_Future(MySQL_Connector& connector, uniptr<MySQL_Connection>&& conn_opt,
                             const MySQL_Table_Structure& table);

    MySQL_Check_Table_Future(MySQL_Connector& connector, const MySQL_Table_Structure& table);

  private:
    virtual
    void
    do_on_abstract_future_initialize()
      override;

    virtual
    void
    do_on_abstract_future_finalize()
      override;

    virtual
    void
    do_on_abstract_task_execute()
      override;

  public:
    MySQL_Check_Table_Future(const MySQL_Check_Table_Future&) = delete;
    MySQL_Check_Table_Future& operator=(const MySQL_Check_Table_Future&) & = delete;
    virtual ~MySQL_Check_Table_Future();

    // Gets the MySQL table structure. This field is set by the constructor.
    const MySQL_Table_Structure&
    table()
      const noexcept
      { return this->m_table;  }

    // Indicates whether the table has been altered after the operation has
    // completed successfully. If `successful()` yields `false`, an exception is
    // thrown, and there is no effect.
    bool
    altered()
      const
      {
        this->check_success();
        return this->m_altered;
      }
  };

}  // namespace poseidon
#endif
