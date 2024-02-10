// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_check_table_future.hpp"
#include "../mysql/mysql_connection.hpp"
#include "../static/mysql_connector.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

void
do_append_column_definition(tinyfmt_str& sql, const MySQL_Table_Structure::Column& column)
  {
    sql << "`" << column.name << "` ";

    switch(column.type)
      {
      case mysql_column_varchar:
        sql << "varchar(255)";
        break;

      case mysql_column_bool:
        sql << "tinyint";
        break;

      case mysql_column_int:
        sql << "int";
        break;

      case mysql_column_int64:
        sql << "bigint";
        break;

      case mysql_column_double:
        sql << "double";
        break;

      case mysql_column_blob:
        sql << "longblob";
        break;

      case mysql_column_datetime:
        sql << "datetime";
        break;

      case mysql_column_auto_increment:
        sql << "bigint AUTO_INCREMENT";
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL data type `$1`"), column.type);
      }

    if(column.nullable == false)
      sql << " NOT NULL";

    if(!column.default_value.is_null())
      sql << " DEFAULT " << column.default_value;
  }

void
do_append_index_name(tinyfmt_str& sql, const MySQL_Table_Structure::Index& index)
  {
    if(ascii_ci_equal(index.name, "PRIMARY"))
      sql << "PRIMARY KEY";
    else
      sql << "INDEX `" << index.name << "`";
  }

void
do_append_index_definition(tinyfmt_str& sql, const MySQL_Table_Structure::Index& index)
  {
    if(ascii_ci_equal(index.name, "PRIMARY"))
      sql << "PRIMARY KEY";
    else if(index.unique)
      sql << "UNIQUE INDEX `" << index.name << "`";
    else
      sql << "INDEX `" << index.name << "`";

    sql << " (";

    for(uint32_t t = 0;  t != index.columns.size();  ++t)
      sql << ((t == 0) ? "`" : ", `") << index.columns[t] << "`";

    sql << ")";
  }

void
do_append_engine(tinyfmt_str& sql, MySQL_Engine_Type engine)
  {
    sql << "ENGINE = '";

    switch(engine)
      {
      case mysql_engine_innodb:
        sql << "InnoDB";
        break;

      case mysql_engine_myisam:
        sql << "MyISAM";
        break;

      case mysql_engine_memory:
        sql << "MEMORY";
        break;

      case mysql_engine_archive:
        sql << "ARCHIVE";
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL engine `$1`"), engine);
      }

    sql << "'";
  }

struct exColumn
  {
    cow_string type;
    bool nullable;
    MySQL_Value default_value;
    cow_string extra;
  };

struct exIndex
  {
    bool unique;
    vector<cow_string> columns;
  };

}  // namespace

MySQL_Check_Table_Future::
MySQL_Check_Table_Future(MySQL_Connector& connector, MySQL_Table_Structure table)
  {
    if(table.name() == "")
      POSEIDON_THROW(("MySQL table has no name"));

    if(table.count_columns() == 0)
      POSEIDON_THROW(("MySQL table has no column"));

    this->m_connector = &connector;
    this->m_res.table = table;
  }

MySQL_Check_Table_Future::
~MySQL_Check_Table_Future()
  {
  }

void
MySQL_Check_Table_Future::
do_on_abstract_future_execute()
  {
    // Get a connection. Before this function returns, the connection should
    // be reset and put back.
    uniptr<MySQL_Connection> conn = this->m_connector->allocate_default_connection();
    const auto conn_guard = ::rocket::make_unique_handle(&conn,
            [=](uniptr<MySQL_Connection>* p) {
              if((*p)->reset())
                this->m_connector->pool_connection(move(*p));
            });

    // Compose a `CREATE TABLE` statement and execute it first. This ensures
    // the table exists for all operations later.
    const auto& table_name = this->m_res.table.name();
    tinyfmt_str sql;
    sql << "CREATE TABLE IF NOT EXISTS `" << table_name << "`";

    for(uint32_t k = 0;  k != this->m_res.table.count_columns();  ++k) {
      const auto& column = this->m_res.table.column(k);

      sql << ((k == 0) ? "\n  (" : ",\n   ");
      do_append_column_definition(sql, column);
    }

    for(uint32_t k = 0;  k != this->m_res.table.count_indexes();  ++k) {
      const auto& index = this->m_res.table.index(k);

      sql << ",\n   ";
      do_append_index_definition(sql, index);
    }

    sql << ")\n  ";
    do_append_engine(sql, this->m_res.table.engine());
    sql << ", CHARSET = 'utf8mb4'";

    POSEIDON_LOG_INFO(("Checking MySQL table:\n$1"), sql.get_string());
    conn->execute(sql.get_string());
    this->m_res.warning_count = conn->warning_count();
    this->m_res.altered = false;

    // Fetch information about existent columns and indexes.
    vector<cow_string> fields;
    vector<MySQL_Value> row;

    sql.clear_string();
    sql << "SHOW COLUMNS FROM `" << table_name << "`";
    conn->execute(sql.get_string());
    conn->fetch_fields(fields);

    map<cow_string, exColumn> excolumns;
    opt<uint32_t> ix_name, ix_type, ix_nullable, ix_default_value, ix_extra;

    for(uint32_t t = 0;  t != fields.size();  ++t)
      if(ascii_ci_equal(fields[t], "field"))
        ix_name = t;
      else if(ascii_ci_equal(fields[t], "type"))
        ix_type = t;
      else if(ascii_ci_equal(fields[t], "null"))
        ix_nullable = t;
      else if(ascii_ci_equal(fields[t], "default"))
        ix_default_value = t;
      else if(ascii_ci_equal(fields[t], "extra"))
        ix_extra = t;

    while(conn->fetch_row(row)) {
      cow_string& name = row.at(ix_name.value()).mut_string();
      if(auto column_config = this->m_res.table.find_column_opt(name))
        name = column_config->name;

      // Save this column.
      exColumn& r = excolumns[name];
      r.type = row.at(ix_type.value()).as_string();
      r.nullable = ascii_ci_equal(row.at(ix_nullable.value()).as_string(), "yes");
      r.default_value = move(row.at(ix_default_value.value()));
      r.extra = row.at(ix_extra.value()).as_string();
    }

    sql.clear_string();
    sql << "SHOW INDEXES FROM `" << table_name << "`";
    conn->execute(sql.get_string());
    conn->fetch_fields(fields);

    map<cow_string, exIndex> exindexes;
    opt<uint32_t> ix_non_unique, ix_column_name, ix_column_seq;

    for(uint32_t t = 0;  t != fields.size();  ++t)
      if(ascii_ci_equal(fields[t], "key_name"))
        ix_name = t;
      else if(ascii_ci_equal(fields[t], "non_unique"))
        ix_non_unique = t;
      else if(ascii_ci_equal(fields[t], "column_name"))
        ix_column_name = t;
      else if(ascii_ci_equal(fields[t], "seq_in_index"))
        ix_column_seq = t;

    while(conn->fetch_row(row)) {
      cow_string& name = row.at(ix_name.value()).mut_string();
      if(auto index_config = this->m_res.table.find_index_opt(name))
        name = index_config->name;

      // Save this index.
      exIndex& r = exindexes[name];
      r.unique = row.at(ix_non_unique.value()).as_integer() == 0;
      size_t column_seq = static_cast<size_t>(row.at(ix_column_seq.value()).as_integer());
      r.columns.resize(::std::max(r.columns.size(), column_seq));
      r.columns.at(column_seq - 1) = row.at(ix_column_name.value()).as_string();
    }

    // Compare the existent columns and indexes with the requested ones,
    // altering the table as necessary.
    sql.clear_string();
    sql << "ALTER TABLE `" << table_name << "`";
    size_t alt_count = 0;

    for(uint32_t k = 0;  k != this->m_res.table.count_columns();  ++k) {
      const auto& column = this->m_res.table.column(k);

      auto ex = excolumns.find(column.name);
      if(ex != excolumns.end()) {
        const auto& r = ex->second;

        switch(column.type)
          {
          case mysql_column_varchar:
            {
              // The type shall be an exact match.
              if(!ascii_ci_equal(r.type, "varchar(255)"))
                goto do_alter_column_;

              if(!column.default_value.is_null()) {
                // The default value is a string and shall be an exact match of
                // the configuration.
                if(!r.default_value.is_string())
                  goto do_alter_column_;

                if(r.default_value.as_string() != column.default_value.as_string())
                  goto do_alter_column_;
              }

              if(!ascii_ci_equal(r.extra, ""))
                goto do_alter_column_;
            }
            break;

          case mysql_column_bool:
            {
              // MySQL does not have a real boolean type, so we use an integer.
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if(!ascii_ci_equal(r.type, "tinyint(1)") && !ascii_ci_equal(r.type, "tinyint"))
                goto do_alter_column_;

              if(!column.default_value.is_null()) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!r.default_value.is_string())
                  goto do_alter_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DI(r.default_value.str_data(), r.default_value.str_length());
                numg.cast_I(def_value, INT64_MIN, INT64_MAX);

                if(def_value != column.default_value.as_integer())
                  goto do_alter_column_;
              }

              if(!ascii_ci_equal(r.extra, ""))
                goto do_alter_column_;
            }
            break;

          case mysql_column_int:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if(!ascii_ci_equal(r.type, "int(11)") && !ascii_ci_equal(r.type, "int"))
                goto do_alter_column_;

              if(!column.default_value.is_null()) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!r.default_value.is_string())
                  goto do_alter_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DI(r.default_value.str_data(), r.default_value.str_length());
                numg.cast_I(def_value, INT64_MIN, INT64_MAX);

                if(def_value != column.default_value.as_integer())
                  goto do_alter_column_;
              }

              if(!ascii_ci_equal(r.extra, ""))
                goto do_alter_column_;
            }
            break;

          case mysql_column_int64:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if(!ascii_ci_equal(r.type, "bigint(20)") && !ascii_ci_equal(r.type, "bigint"))
                goto do_alter_column_;

              if(!column.default_value.is_null()) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!r.default_value.is_string())
                  goto do_alter_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DI(r.default_value.str_data(), r.default_value.str_length());
                numg.cast_I(def_value, INT64_MIN, INT64_MAX);

                if(def_value != column.default_value.as_integer())
                  goto do_alter_column_;
              }

              if(!ascii_ci_equal(r.extra, ""))
                goto do_alter_column_;
            }
            break;

          case mysql_column_double:
            {
              // The type shall be an exact match.
              if(!ascii_ci_equal(r.type, "double"))
                goto do_alter_column_;

              if(!column.default_value.is_null()) {
                // The default value is a double and shall be an exact match of
                // the configuration. The MySQL server returns the default as a
                // generic string, so parse it first.
                if(!r.default_value.is_string())
                  goto do_alter_column_;

                double def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DD(r.default_value.str_data(), r.default_value.str_length());
                numg.cast_D(def_value, -DBL_MAX, DBL_MAX);

                if(def_value != column.default_value.as_double())
                  goto do_alter_column_;
              }

              if(!ascii_ci_equal(r.extra, ""))
                goto do_alter_column_;
            }
            break;

          case mysql_column_blob:
            {
              // The type shall be an exact match. BLOB and TEXT fields cannot
              // have a default value, so there is no need to check it.
              if(!ascii_ci_equal(r.type, "longblob"))
                goto do_alter_column_;

              if(!ascii_ci_equal(r.extra, ""))
                goto do_alter_column_;
            }
            break;

          case mysql_column_datetime:
            {
              // The type shall be an exact match.
              if(!ascii_ci_equal(r.type, "datetime"))
                goto do_alter_column_;

              if(!column.default_value.is_null()) {
                // The default value is a broken-down date/time and shall be an
                // exact match of the configuration. The MySQL server returns
                // the default as a generic string, so parse it first.
                if(!r.default_value.is_string())
                  goto do_alter_column_;

                struct ::tm rtm = { };
                ::strptime(r.default_value.str_data(), "%Y-%m-%d %H:%M:%S", &rtm);
                rtm.tm_isdst = -1;

                struct ::tm ctm = { };
                const auto& cmyt = column.default_value.as_mysql_time();
                ctm.tm_year = static_cast<int>(cmyt.year) - 1900;
                ctm.tm_mon = static_cast<int>(cmyt.month) - 1;
                ctm.tm_mday = static_cast<int>(cmyt.day);
                ctm.tm_hour = static_cast<int>(cmyt.hour);
                ctm.tm_min = static_cast<int>(cmyt.minute);
                ctm.tm_sec = static_cast<int>(cmyt.second);
                rtm.tm_isdst = -1;

                if(::timegm(&rtm) != ::timegm(&ctm))
                  goto do_alter_column_;
              }

              if(!ascii_ci_equal(r.extra, ""))
                goto do_alter_column_;
            }
            break;

          case mysql_column_auto_increment:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              // Auto-increment fields cannot have a default value, so there is
              // no need to check it.
              if(!ascii_ci_equal(r.type, "bigint(20)") && !ascii_ci_equal(r.type, "bigint"))
                goto do_alter_column_;

              if(!ascii_ci_equal(r.extra, "AUTO_INCREMENT"))
                goto do_alter_column_;
            }
            break;

          default:
            POSEIDON_THROW(("Invalid MySQL data type `$1`"), column.type);
          }

        if(r.nullable != column.nullable)
          goto do_alter_column_;

        if(r.default_value.is_null() != column.default_value.is_null())
          goto do_alter_column_;

        POSEIDON_LOG_DEBUG(("Verified: table `$1` column `$2`"), table_name, column.name);
        continue;
      }

      // The column does not match the definition, so modify it.
  do_alter_column_:
      sql << ((alt_count ++ == 0) ? "\n  " : ",\n  ");

      sql << ((ex == excolumns.end()) ? "ADD" : "MODIFY") << " COLUMN ";
      do_append_column_definition(sql, column);
    }

    for(uint32_t k = 0;  k != this->m_res.table.count_indexes();  ++k) {
      const auto& index = this->m_res.table.index(k);

      auto ex = exindexes.find(index.name);
      if(ex != exindexes.end()) {
        const auto& r = ex->second;

        if(r.columns.size() != index.columns.size())
          goto do_alter_index_;

        for(uint32_t t = 0;  t != index.columns.size();  ++t)
          if(r.columns[t] != index.columns[t])
            goto do_alter_index_;

        if(r.unique != index.unique)
          goto do_alter_index_;

        POSEIDON_LOG_DEBUG(("Verified: table `$1` index `$2`"), table_name, index.name);
        continue;
      }

      // The index does not match the definition, so modify it.
  do_alter_index_:
      sql << ((alt_count ++ == 0) ? "\n  " : ",\n  ");

      if(ex != exindexes.end()) {
        sql << "DROP ";
        do_append_index_name(sql, index);
        sql << ",\n  ";
      }

      sql << "ADD ";
      do_append_index_definition(sql, index);
    }

    if(alt_count == 0)
      return;

    POSEIDON_LOG_WARN(("Updating MySQL table structure:", "$1"), sql.get_string());
    conn->execute(sql.get_string());
    this->m_res.altered = true;
  }

void
MySQL_Check_Table_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
