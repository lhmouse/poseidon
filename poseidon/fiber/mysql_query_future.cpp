// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_query_future.hpp"
#include "../mysql/mysql_connection.hpp"
#include "../static/mysql_connector.hpp"
#include "../utils.hpp"
namespace poseidon {

MySQL_Query_Future::
MySQL_Query_Future(MySQL_Connector& connector, uniptr<MySQL_Connection>&& conn_opt,
                   const cow_string& stmt, const cow_vector<MySQL_Value>& stmt_args)
  {
    this->m_ctr = &connector;
    this->m_conn = move(conn_opt);
    this->m_res.stmt = stmt;
    this->m_res.stmt_args = stmt_args;
  }

MySQL_Query_Future::
MySQL_Query_Future(MySQL_Connector& connector, const cow_string& stmt,
                   const cow_vector<MySQL_Value>& stmt_args)
  {
    this->m_ctr = &connector;
    this->m_res.stmt = stmt;
    this->m_res.stmt_args = stmt_args;
  }

MySQL_Query_Future::
~MySQL_Query_Future()
  {
  }

void
MySQL_Query_Future::
do_on_abstract_future_initialize()
  {
    if(!this->m_conn)
      this->m_conn = this->m_ctr->allocate_default_connection();

    this->m_conn->execute(this->m_res.stmt, this->m_res.stmt_args);

    this->m_res.warning_count = this->m_conn->warning_count();
    this->m_res.affected_rows = this->m_conn->affected_rows();
    this->m_res.insert_id = this->m_conn->insert_id();
    this->m_conn->fetch_fields(this->m_res.result_fields);

    cow_vector<MySQL_Value> row;
    while(this->m_conn->fetch_row(row))
      this->m_res.result_rows.push_back(move(row));
  }

void
MySQL_Query_Future::
do_on_abstract_future_finalize()
  {
    if(!this->m_conn)
      return;

    if(this->m_conn->reset())
      this->m_ctr->pool_connection(move(this->m_conn));
  }

void
MySQL_Query_Future::
do_on_abstract_task_execute()
  {
    this->do_abstract_future_initialize_once();
  }

}  // namespace poseidon
