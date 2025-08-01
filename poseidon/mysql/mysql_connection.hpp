// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_CONNECTION_
#define POSEIDON_MYSQL_MYSQL_CONNECTION_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../details/mysql_fwd.hpp"
namespace poseidon {

class MySQL_Connection
  {
  private:
    friend class MySQL_Connector;

    cow_string m_service_uri;
    cow_string m_password;
    bool m_connected;
    bool m_reset_clear;

    scoped_MYSQL m_mysql;
    uniptr_MYSQL_STMT m_stmt;
    uniptr_MYSQL_RES m_meta;
    uniptr_MYSQL_RES m_res;

  public:
    // Sets connection parameters. This function does not attempt to connect
    // to the server, and is not blocking.
    MySQL_Connection(const cow_string& service_uri, const cow_string& password);

  public:
    MySQL_Connection(const MySQL_Connection&) = delete;
    MySQL_Connection& operator=(const MySQL_Connection&) & = delete;
    ~MySQL_Connection();

    // Gets the URI from the constructor.
    const cow_string&
    service_uri()
      const noexcept
      { return this->m_service_uri;  }

    // Resets the connection so it can be reused by another thread. This is a
    // blocking functions. DO NOT ATTEMPT TO REUSE THE CONNECTION IF THIS
    // FUNCTION RETURNS `false`.
    // Returns whether the connection may be safely reused.
    bool
    reset()
      noexcept;

    // Executes a query. `stmt` shall be a SQL statement with placeholders, like
    // `INSERT INTO test_table(col1,col2,col3) VALUES(?,?,?)"`. `args` supplies
    // values for those placeholders. If a connection to the server has not been
    // established yet, this function initiates a new connection before the query
    // is executed.
    void
    execute(const cow_string& stmt, const cow_vector<MySQL_Value>& args);

    // Gets the number of warnings on the current connection. This function is
    // primarily useful for statements that produce warnings upon success.
    uint32_t
    warning_count()
      const noexcept;

    // Gets the number of rows that match the previous statement. This function
    // must be called after `execute()`.
    uint64_t
    match_count()
      const noexcept;

    // Gets the auto-increment id that is generated by the last statement. This
    // function must be called after `execute()`.
    uint64_t
    insert_id()
      const noexcept;

    // Fetches the names of all fields of the result set. This function must be
    // called after `execute()`. `output` is cleared before fetching any data. If
    // the query has produced a result and all fields have been successfully
    // fetched, `true` is returned. If there is no result, `false` is returned.
    bool
    fetch_fields(cow_vector<cow_string>& output);

    // Fetches a row of the result from the previous query. This function must be
    // called after `execute()`. `output` is cleared before fetching any data. If
    // the query has produced a result and a row has been successfully fetched,
    // `true` is returned. If there is no result or the end of result has been
    // reached, `false` is returned.
    bool
    fetch_row(cow_vector<MySQL_Value>& output);
  };

}  // namespace poseidon
#endif
