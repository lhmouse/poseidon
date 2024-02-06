// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_connection.hpp"
#include "mysql_value.hpp"
#include "enums.hpp"
#include "../utils.hpp"
namespace poseidon {

MySQL_Connection::
MySQL_Connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd, cow_stringR db)
  :
    m_server(server), m_user(user), m_passwd(passwd), m_db(db), m_port(port),
    m_connected(false), m_reset_clear(true)
  {
  }

MySQL_Connection::
~MySQL_Connection()
  {
  }

bool
MySQL_Connection::
reset() noexcept
  {
    if(!this->m_connected)
      return true;

    // Discard the current result set.
    this->m_res.reset();
    this->m_meta.reset();
    this->m_stmt.reset();
    this->m_reset_clear = ::mysql_reset_connection(this->m_mysql) == 0;
    return this->m_reset_clear;
  }

void
MySQL_Connection::
execute(cow_stringR stmt, const MySQL_Value* args_opt, size_t nargs)
  {
    if(!this->m_connected) {
      // Try connecting to the server now.
      if(!::mysql_real_connect(this->m_mysql, this->m_server.safe_c_str(), this->m_user.safe_c_str(),
                               this->m_passwd.safe_c_str(), this->m_db.safe_c_str(), this->m_port,
                               nullptr, CLIENT_IGNORE_SIGPIPE | CLIENT_REMEMBER_OPTIONS))
        POSEIDON_THROW((
            "Could not connect to MySQL server: ERROR $1: $2",
            "[`mysql_real_connect()` failed]"),
            ::mysql_errno(this->m_mysql), ::mysql_error(this->m_mysql));

      // Erase the password for safety.
      cow_string passwd;
      passwd.swap(this->m_passwd);
      ::explicit_bzero(passwd.mut_data(), passwd.size());
      this->m_connected = true;

      POSEIDON_LOG_INFO((
          "Connected to MySQL server `$3@$1:$2` using default database `$4`"),
          this->m_server, this->m_port, this->m_user, this->m_db);
    }

    // Discard the current result set.
    this->m_res.reset();
    this->m_meta.reset();
    this->m_stmt.reset();
    this->m_reset_clear = false;

    // Create a prepared statement.
    if(!this->m_stmt.reset(::mysql_stmt_init(this->m_mysql)))
      POSEIDON_THROW((
          "Could not create MySQL statement: ERROR $1: $2",
          "[`mysql_stmt_init()` failed]"),
          ::mysql_errno(this->m_mysql), ::mysql_error(this->m_mysql));

    if(::mysql_stmt_prepare(this->m_stmt, stmt.safe_c_str(), stmt.length()) != 0)
      POSEIDON_THROW((
          "Could not prepare MySQL statement: ERROR $1: $2",
          "[`mysql_stmt_prepare()` failed]"),
          ::mysql_stmt_errno(this->m_stmt), ::mysql_stmt_error(this->m_stmt));

    // Bind arguments.
    unsigned long nparams = ::mysql_stmt_param_count(this->m_stmt);
    if(nargs < nparams)
      POSEIDON_THROW((
          "No enough arguments for MySQL statement (`$1` < `$2`)"),
          nargs, nparams);

    vector<::MYSQL_BIND> binds(nparams);

    for(unsigned col = 0;  col != nparams;  ++col) {
      if(args_opt[col].is_null()) {
        // `NULL`
        binds[col].buffer_type = MYSQL_TYPE_NULL;
      }
      else if(args_opt[col].is_integer()) {
        // `BIGINT`
        binds[col].buffer_type = MYSQL_TYPE_LONGLONG;
        binds[col].buffer = const_cast<int64_t*>(&(args_opt[col].as_integer()));
        binds[col].buffer_length = sizeof(int64_t);
      }
      else if(args_opt[col].is_double()) {
        // `DOUBLE`
        binds[col].buffer_type = MYSQL_TYPE_DOUBLE;
        binds[col].buffer = const_cast<double*>(&(args_opt[col].as_double()));
        binds[col].buffer_length = sizeof(double);
      }
      else if(args_opt[col].is_string()) {
        // `BLOB`
        binds[col].buffer_type = MYSQL_TYPE_BLOB;
        binds[col].buffer = const_cast<char*>(args_opt[col].str_data());
        binds[col].buffer_length = args_opt[col].str_length();
      }
      else if(args_opt[col].is_mysql_time()) {
        // `DATETIME`
        binds[col].buffer_type = MYSQL_TYPE_DATETIME;
        binds[col].buffer = const_cast<::MYSQL_TIME*>(&(args_opt[col].as_mysql_time()));
        binds[col].buffer_length = sizeof(::MYSQL_TIME);
      }
      else
        POSEIDON_THROW(("Unhandled `MySQL_Value`: $1"), args_opt[col]);
    }

    if(::mysql_stmt_bind_param(this->m_stmt, binds.data()) != 0)
      POSEIDON_THROW((
          "Could not bind arguments onto MySQL statement: ERROR $1: $2",
          "[`mysql_stmt_bind_param()` failed]"),
          ::mysql_stmt_errno(this->m_stmt), ::mysql_stmt_error(this->m_stmt));

    // Execute the bound statement. While this function is blocking, we don't
    // fetch its result here; it's done in `fetch()`.
    if(::mysql_stmt_execute(this->m_stmt) != 0)
      POSEIDON_THROW((
          "Could not execute MySQL statement: ERROR $1: $2",
          "[`mysql_stmt_execute()` failed]"),
          ::mysql_stmt_errno(this->m_stmt), ::mysql_stmt_error(this->m_stmt));
  }

void
MySQL_Connection::
execute(cow_stringR stmt, const vector<MySQL_Value>& args)
  {
    this->execute(stmt, args.data(), args.size());
  }

void
MySQL_Connection::
execute(cow_stringR stmt)
  {
    this->execute(stmt, nullptr, 0);
  }

POSEIDON_VISIBILITY_HIDDEN
::MYSQL_FIELD*
MySQL_Connection::
do_metadata_for_field_opt(unsigned col) noexcept
  {
    // Get the metadata for this result. This is done at most once.
    if(!this->m_meta)
      this->m_meta.reset(::mysql_stmt_result_metadata(this->m_stmt));

    if(!this->m_meta)
      return nullptr;

    return ::mysql_fetch_field_direct(this->m_meta, col);
  }

bool
MySQL_Connection::
fetch_fields(vector<cow_string>& output)
  {
    output.clear();

    if(!this->m_stmt)
      POSEIDON_THROW(("No query has been executed"));

    unsigned long nfields = ::mysql_stmt_field_count(this->m_stmt);
    output.resize(nfields);

    for(unsigned col = 0;  col != nfields;  ++col) {
      ::MYSQL_FIELD* field = this->do_metadata_for_field_opt(col);
      if(!field)
        continue;

      // Copy the name of this field from metadata.
      output[col].assign(field->name, field->name_length);
    }
    return true;
  }

bool
MySQL_Connection::
fetch_row(vector<MySQL_Value>& output)
  {
    output.clear();

    if(!this->m_stmt)
      POSEIDON_THROW(("No query has been executed"));

    unsigned long nfields = ::mysql_stmt_field_count(this->m_stmt);
    vector<::MYSQL_BIND> binds(nfields);
    output.resize(nfields);

    for(unsigned col = 0;  col != nfields;  ++col) {
      // This is the omnipotent type for all values, used either when there is
      // no metadata so we can't know the type of this field, or when the MySQL
      // documentation says a `char[]` shall be used.
      binds[col].buffer_type = MYSQL_TYPE_BLOB;

      // XXX: What are these fields used for? Why aren't they used by default?
      binds[col].length = &(binds[col].length_value);
      binds[col].error = &(binds[col].error_value);
      binds[col].is_null = &(binds[col].is_null_value);

      ::MYSQL_FIELD* field = this->do_metadata_for_field_opt(col);
      if(!field)
        continue;

      if(field->type == MYSQL_TYPE_NULL) {
        // no output
        binds[col].buffer_type = MYSQL_TYPE_NULL;
      }
      else if(is_any_of(field->type, { MYSQL_TYPE_TINY, MYSQL_TYPE_SHORT, MYSQL_TYPE_INT24,
                                       MYSQL_TYPE_LONG, MYSQL_TYPE_LONGLONG })) {
        // `BIGINT`
        binds[col].buffer_type = MYSQL_TYPE_LONGLONG;
        output[col].set_integer(0);
        binds[col].buffer = &(output[col].mut_integer());
        binds[col].buffer_length = sizeof(int64_t);
      }
      else if(is_any_of(field->type, { MYSQL_TYPE_FLOAT, MYSQL_TYPE_DOUBLE })) {
        // `DOUBLE`
        binds[col].buffer_type = MYSQL_TYPE_DOUBLE;
        output[col].set_double(0);
        binds[col].buffer = &(output[col].mut_double());
        binds[col].buffer_length = sizeof(double);
      }
      else if(is_any_of(field->type, { MYSQL_TYPE_TIME, MYSQL_TYPE_DATE, MYSQL_TYPE_DATETIME,
                                       MYSQL_TYPE_TIMESTAMP })) {
        // `DATETIME`
        binds[col].buffer_type = MYSQL_TYPE_DATETIME;
        output[col].set_mysql_datetime(0, 0, 0, 0, 0, 0, 0);
        binds[col].buffer = &(output[col].mut_mysql_time());
        binds[col].buffer_length = sizeof(::MYSQL_TIME);
      }
      else
        binds[col].buffer_type = MYSQL_TYPE_BLOB;
    }

    if(::mysql_stmt_bind_result(this->m_stmt, binds.data()) != 0)
      POSEIDON_THROW((
          "Could not bind output columns onto MySQL statement: ERROR $1: $2",
          "[`mysql_stmt_bind_result()` failed]"),
          ::mysql_stmt_errno(this->m_stmt), ::mysql_stmt_error(this->m_stmt));

    // Request the next row.
    int status = ::mysql_stmt_fetch(this->m_stmt);
    if(status == MYSQL_NO_DATA)
      return false;
    else if(status == 1)
      POSEIDON_THROW((
          "Could not fetch MySQL result: ERROR $1: $2",
          "[`mysql_stmt_fetch()` failed]"),
          ::mysql_stmt_errno(this->m_stmt), ::mysql_stmt_error(this->m_stmt));

    for(unsigned col = 0;  col != nfields;  ++col) {
      if(binds[col].is_null_value) {
        // Set it to an explicit `NULL`.
        output[col].clear();
      }
      else if(ROCKET_UNEXPECT(binds[col].buffer_type == MYSQL_TYPE_BLOB)) {
        // Only its length has been fetched, so fetch its data now.
        output[col].set_string("", 0);
        auto& outs = output[col].mut_string();
        outs.resize(binds[col].length_value, '*');
        binds[col].buffer = outs.mut_data();
        binds[col].buffer_length = outs.size();
        ::mysql_stmt_fetch_column(this->m_stmt, &(binds[col]), col, 0);
      }
    }
    return true;
  }

uint64_t
MySQL_Connection::
affected_rows() const
  {
    if(!this->m_stmt)
      POSEIDON_THROW(("No query has been executed"));

    return ::mysql_stmt_affected_rows(this->m_stmt);
  }

uint64_t
MySQL_Connection::
insert_id() const
  {
    if(!this->m_stmt)
      POSEIDON_THROW(("No query has been executed"));

    return ::mysql_stmt_insert_id(this->m_stmt);
  }

}  // namespace poseidon
