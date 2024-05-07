// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_MYSQL_CHECK_TABLE_FUTURE_
#define POSEIDON_FIBER_MYSQL_CHECK_TABLE_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../mysql/mysql_table_structure.hpp"
namespace poseidon {

class MySQL_Check_Table_Future
  :
    public Abstract_Future
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
    MySQL_Connector* m_ctr;
    uniptr<MySQL_Connection> m_conn;
    Result m_res;

  public:
    // Constructs a future for a table check request. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Executor`.
    // This future will become ready once the check is complete.
    MySQL_Check_Table_Future(MySQL_Connector& connector, MySQL_Table_Structure table);

  private:
    virtual
    void
    do_on_abstract_future_execute() override;

    virtual
    void
    do_on_abstract_future_finalize() noexcept override;

  public:
    MySQL_Check_Table_Future(const MySQL_Check_Table_Future&) = delete;
    MySQL_Check_Table_Future& operator=(const MySQL_Check_Table_Future&) & = delete;
    virtual ~MySQL_Check_Table_Future();

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
