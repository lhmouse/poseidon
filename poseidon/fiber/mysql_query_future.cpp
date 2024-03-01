// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_query_future.hpp"
#include "../mysql/mysql_connection.hpp"
#include "../static/mysql_connector.hpp"
#include "../utils.hpp"
namespace poseidon {

MySQL_Query_Future::
MySQL_Query_Future(MySQL_Connector& connector, cow_stringR stmt)
  {
    this->m_connector = &connector;
    this->m_res.stmt = stmt;
  }

MySQL_Query_Future::
MySQL_Query_Future(MySQL_Connector& connector, cow_stringR stmt, vector<MySQL_Value> stmt_args)
  {
    this->m_connector = &connector;
    this->m_res.stmt = stmt;
    this->m_res.stmt_args = move(stmt_args);
  }

MySQL_Query_Future::
~MySQL_Query_Future()
  {
  }

void
MySQL_Query_Future::
do_on_abstract_future_execute()
  {
    // Get a connection. Before this function returns, the connection should
    // be reset and put back.
    uniptr<MySQL_Connection> conn = this->m_connector->allocate_default_connection();
    const auto conn_guard = make_unique_handle(&conn,
        [=](uniptr<MySQL_Connection>* p) {
          if((*p)->reset())
            this->m_connector->pool_connection(move(*p));
        });

    // Execute the statement.
    conn->execute(this->m_res.stmt, this->m_res.stmt_args.data(), this->m_res.stmt_args.size());

    // Fetch result metadata.
    this->m_res.warning_count = conn->warning_count();
    this->m_res.affected_rows = conn->affected_rows();
    this->m_res.insert_id = conn->insert_id();
    conn->fetch_fields(this->m_res.result_fields);

    // Fetch result rows.
    vector<MySQL_Value> row;
    while(conn->fetch_row(row))
      this->m_res.result_rows.push_back(move(row));

    POSEIDON_LOG_DEBUG((
        "Finished SQL statement: $1",
        "$2 row(s) affected, $3 row(s) returned"),
        this->m_res.stmt,
        this->m_res.affected_rows, this->m_res.result_rows.size());
  }

void
MySQL_Query_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
