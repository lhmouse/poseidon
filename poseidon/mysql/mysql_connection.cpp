// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_connection.hpp"
#include "mysql_value.hpp"
#include "../utils.hpp"
namespace poseidon {

MySQL_Connection::
MySQL_Connection(cow_stringR server, uint16_t port, cow_stringR db, cow_stringR user, cow_stringR passwd)
  {
    this->m_server = server;
    this->m_port = port;
    this->m_db = db;
    this->m_user = user;

    server.safe_c_str();
    db.safe_c_str();
    user.safe_c_str();
    passwd.safe_c_str();

    this->m_connected = false;
    this->m_reset_clear = true;
    this->m_passwd_mask = 0x80000000U | random_uint32();
    this->m_passwd = passwd;
    mask_string(this->m_passwd.mut_data(), this->m_passwd.size(), nullptr, this->m_passwd_mask);
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
    this->m_reset_clear = false;

    // Reset the connection on the server. This is a blocking function.
    if(::mysql_reset_connection(this->m_mysql) != 0)
      return false;

    this->m_reset_clear = true;
    return true;
  }

uint32_t
MySQL_Connection::
warning_count() const noexcept
  {
    return ::mysql_warning_count(this->m_mysql);
  }

void
MySQL_Connection::
execute(cow_stringR stmt, const MySQL_Value* args_opt, size_t nargs)
  {
    if(stmt.empty())
      POSEIDON_THROW(("Empty SQL statement"));

    if(!this->m_connected) {
      // Try connecting to the server now.
      mask_string(this->m_passwd.mut_data(), this->m_passwd.size(), nullptr, this->m_passwd_mask);

      ::MYSQL* rc = ::mysql_real_connect(this->m_mysql, this->m_server.c_str(),
                        this->m_user.c_str(), this->m_passwd.c_str(), this->m_db.c_str(),
                        this->m_port, nullptr, CLIENT_IGNORE_SIGPIPE | CLIENT_REMEMBER_OPTIONS);

      this->m_passwd_mask = 0x80000000U | random_uint32();
      mask_string(this->m_passwd.mut_data(), this->m_passwd.size(), nullptr, this->m_passwd_mask);

      if(!rc)
        POSEIDON_THROW((
            "Could not connect to MySQL server: ERROR $1: $2",
            "[`mysql_real_connect()` failed]"),
            ::mysql_errno(this->m_mysql), ::mysql_error(this->m_mysql));

      cow_string().swap(this->m_passwd);
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

    for(unsigned col = 0;  col != nparams;  ++col)
      switch(args_opt[col].type())
        {
        case mysql_value_null:
          binds[col].buffer_type = MYSQL_TYPE_NULL;
          break;

        case mysql_value_integer:
          binds[col].buffer_type = MYSQL_TYPE_LONGLONG;
          binds[col].buffer = const_cast<int64_t*>(&(args_opt[col].as_integer()));
          binds[col].buffer_length = sizeof(int64_t);
          break;

        case mysql_value_double:
          binds[col].buffer_type = MYSQL_TYPE_DOUBLE;
          binds[col].buffer = const_cast<double*>(&(args_opt[col].as_double()));
          binds[col].buffer_length = sizeof(double);
          break;

        case mysql_value_blob:
          binds[col].buffer_type = MYSQL_TYPE_BLOB;
          binds[col].buffer = const_cast<char*>(args_opt[col].blob_data());
          binds[col].buffer_length = args_opt[col].blob_size();
          break;

        case mysql_value_datetime:
          {
            const auto& input_mdt = args_opt[col].m_stor.as<DateTime_with_MYSQL_TIME>();
            ::MYSQL_TIME& myt = input_mdt.get_mysql_time();

            ::timespec ts;
            timespec_from_system_time(ts, input_mdt.datetime.as_system_time());
            ::tm tm;
            ::gmtime_r(&(ts.tv_sec), &tm);

            myt.year = static_cast<uint32_t>(tm.tm_year) + 1900;
            myt.month = static_cast<uint32_t>(tm.tm_mon) + 1;
            myt.day = static_cast<uint32_t>(tm.tm_mday);
            myt.hour = static_cast<uint32_t>(tm.tm_hour);
            myt.minute = static_cast<uint32_t>(tm.tm_min);
            myt.second = static_cast<uint32_t>(tm.tm_sec);
            myt.second_part = static_cast<uint32_t>(ts.tv_nsec) / 1000000;
            myt.time_type = MYSQL_TIMESTAMP_DATETIME;

            binds[col].buffer_type = MYSQL_TYPE_DATETIME;
            binds[col].buffer = &myt;
            binds[col].buffer_length = sizeof(::MYSQL_TIME);
          }
          break;

        default:
          POSEIDON_THROW(("Unknown MySQL value type: $1"), args_opt[col]);
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

bool
MySQL_Connection::
fetch_fields(vector<cow_string>& output)
  {
    output.clear();

    if(!this->m_stmt)
      POSEIDON_THROW(("No query has been executed"));

    if(!this->m_meta)
      this->m_meta.reset(::mysql_stmt_result_metadata(this->m_stmt));

    if(!this->m_meta)
      return false;

    ::MYSQL_FIELD* meta_fields = ::mysql_fetch_fields(this->m_meta);
    unsigned long nfields = ::mysql_stmt_field_count(this->m_stmt);

    output.resize(nfields);
    for(unsigned col = 0;  col != nfields;  ++col)
      output[col].assign(meta_fields[col].name, meta_fields[col].name_length);

    return true;
  }

bool
MySQL_Connection::
fetch_row(vector<MySQL_Value>& output)
  {
    output.clear();

    if(!this->m_stmt)
      POSEIDON_THROW(("No query has been executed"));

    ::MYSQL_FIELD* meta_fields = nullptr;
    unsigned long nfields = ::mysql_stmt_field_count(this->m_stmt);
    vector<::MYSQL_BIND> binds(nfields);

    if(!this->m_meta)
      this->m_meta.reset(::mysql_stmt_result_metadata(this->m_stmt));

    if(this->m_meta)
      meta_fields = ::mysql_fetch_fields(this->m_meta);

    output.resize(nfields);
    for(unsigned col = 0;  col != nfields;  ++col) {
      // Prepare output buffers. When there is no metadata o we can't know the
      // type of this field, the output is written as an omnipotent string.
      uint32_t field_type = meta_fields ? meta_fields[col].type : MYSQL_TYPE_BLOB;
      switch(field_type)
        {
        case MYSQL_TYPE_NULL:
          binds[col].buffer_type = MYSQL_TYPE_NULL;
          binds[col].buffer = nullptr;
          binds[col].buffer_length = 0;
          break;

        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
          binds[col].buffer_type = MYSQL_TYPE_LONGLONG;
          binds[col].buffer = &(output[col].mut_integer());
          binds[col].buffer_length = sizeof(int64_t);
          break;

        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
          binds[col].buffer_type = MYSQL_TYPE_DOUBLE;
          binds[col].buffer = &(output[col].mut_double());
          binds[col].buffer_length = sizeof(double);
          break;

        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
          binds[col].buffer_type = MYSQL_TYPE_DATETIME;
          binds[col].buffer = &(output[col].m_stor.emplace<DateTime_with_MYSQL_TIME>().get_mysql_time());
          binds[col].buffer_length = sizeof(::MYSQL_TIME);
          break;

        default:
          // Reserve a small amount of memory for BLOB fields. If it's not
          // large enough, it will be extended later.
          binds[col].buffer_type = MYSQL_TYPE_LONG_BLOB;
          output[col].mut_blob().resize(64);
          binds[col].buffer = output[col].mut_blob().mut_data();
          binds[col].buffer_length = output[col].mut_blob().size();
          break;
        }

      // XXX: What are these fields used for? Why aren't they used by default?
      binds[col].length = &(binds[col].length_value);
      binds[col].error = &(binds[col].error_value);
      binds[col].is_null = &(binds[col].is_null_value);
    }

    if(::mysql_stmt_bind_result(this->m_stmt, binds.data()) != 0)
      POSEIDON_THROW((
          "Could not bind output columns onto MySQL statement: ERROR $1: $2",
          "[`mysql_stmt_bind_result()` failed]"),
          ::mysql_stmt_errno(this->m_stmt), ::mysql_stmt_error(this->m_stmt));

    // Request the next row.
    int status = ::mysql_stmt_fetch(this->m_stmt);
    if(status == 1)
      POSEIDON_THROW((
          "Could not fetch MySQL result: ERROR $1: $2",
          "[`mysql_stmt_fetch()` failed]"),
          ::mysql_stmt_errno(this->m_stmt), ::mysql_stmt_error(this->m_stmt));

    if(status == MYSQL_NO_DATA)
      return false;

    for(unsigned col = 0;  col != output.size();  ++col)
      if(binds[col].is_null_value) {
        // Set it to an explicit `NULL`.
        output[col].clear();
      }
      else if(binds[col].buffer_type == MYSQL_TYPE_DATETIME) {
        // Assemble the timestamp.
        auto& output_mdt = output[col].m_stor.mut<DateTime_with_MYSQL_TIME>();
        ::MYSQL_TIME& myt = output_mdt.get_mysql_time();

        ::tm tm;
        tm.tm_year = static_cast<int>(myt.year) - 1900;
        tm.tm_mon = static_cast<int>(myt.month) - 1;
        tm.tm_mday = static_cast<int>(myt.day);
        tm.tm_hour = static_cast<int>(myt.hour);
        tm.tm_min = static_cast<int>(myt.minute);
        tm.tm_sec = static_cast<int>(myt.second);
        tm.tm_isdst = 0;

        ::time_t secs = ::timegm(&tm);
        uint32_t msecs = static_cast<uint32_t>(myt.second_part);
        output_mdt.datetime = system_clock::from_time_t(secs) + milliseconds(msecs);
      }
      else if(binds[col].buffer_type == MYSQL_TYPE_LONG_BLOB) {
        // Check whether the value has been truncated.
        auto& output_str = output[col].mut_blob();
        output_str.resize(binds[col].length_value);

        if(binds[col].error_value) {
          // Refetch.
          binds[col].buffer = output_str.mut_data();
          binds[col].buffer_length = output_str.size();
          ::mysql_stmt_fetch_column(this->m_stmt, &(binds[col]), col, 0);
        }
      }

    // Return the result into `output`.
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
