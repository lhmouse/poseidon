// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_check_table_future.hpp"
#include "../mysql/mysql_connection.hpp"
#include "../static/mysql_connector.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

void
do_append_column_definition(tinyfmt_str& sql, const MySQL_Table_Column& column)
  {
    sql << "`" << column.name << "` ";

    switch(column.type)
      {
      case mysql_column_dropped:
        ROCKET_UNREACHABLE();

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
        sql << "datetime(6)";
        break;

      case mysql_column_auto_increment:
        sql << "bigint AUTO_INCREMENT NOT NULL";
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL column type `$1`"), column.type);
      }

    if(column.type != mysql_column_auto_increment) {
      // An auto-increment field is always non-null and has no default value so
      // these hardly make any sense.
      if(!column.nullable)
        sql << " NOT NULL";

      if(!column.default_value.is_null())
        sql << " DEFAULT " << column.default_value;
    }
  }

void
do_append_index_definition(tinyfmt_str& sql, const MySQL_Table_Index& index)
  {
    if(index.name == "PRIMARY")
      sql << "PRIMARY KEY";
    else
      switch(index.type)
        {
        case mysql_index_dropped:
          ROCKET_UNREACHABLE();

        case mysql_index_unique:
          sql << "UNIQUE INDEX `" << index.name << "`";
          break;

        case mysql_index_multi:
          sql << "INDEX `" << index.name << "`";
          break;

        default:
          POSEIDON_THROW(("Invalid MySQL index type `$1`"), index.type);
        }

    size_t ncolumns = 0;
    sql << " (";

    for(const auto& col_name : index.columns) {
      if(++ ncolumns != 1)
        sql << ", ";
      sql << "`" << col_name << "`";
    }

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
    bool multi;
    ::std::vector<cow_string> columns;
  };

}  // namespace

MySQL_Check_Table_Future::
MySQL_Check_Table_Future(MySQL_Connector& connector, uniptr<MySQL_Connection>&& conn_opt,
                         const MySQL_Table_Structure& table)
  {
    this->m_ctr = &connector;
    this->m_conn = move(conn_opt);
    this->m_table = table;
  }

MySQL_Check_Table_Future::
MySQL_Check_Table_Future(MySQL_Connector& connector, const MySQL_Table_Structure& table)
  {
    this->m_ctr = &connector;
    this->m_table = table;
  }

MySQL_Check_Table_Future::
~MySQL_Check_Table_Future()
  {
  }

void
MySQL_Check_Table_Future::
do_on_abstract_future_initialize()
  {
    if(!this->m_conn)
      this->m_conn = this->m_ctr->allocate_default_connection();

    // Compose a `CREATE TABLE` statement and execute it first. This ensures
    // the table exists for all operations later.
    tinyfmt_str sql;
    sql << "CREATE TABLE IF NOT EXISTS `" << this->m_table.name << "`\n  (";
    size_t ncolumns = 0;

    for(const auto& column : this->m_table.columns)
      if(column.type != mysql_column_dropped) {
        if(++ ncolumns != 1)
          sql << ",\n   ";
        do_append_column_definition(sql, column);
      }

    for(const auto& index : this->m_table.indexes)
      if(index.type != mysql_index_dropped) {
        sql << ",\n   ";
        do_append_index_definition(sql, index);
      }

    sql << ")\n  ";
    do_append_engine(sql, this->m_table.engine);
    sql << ",\n  CHARSET = 'utf8mb4'";

    for(const auto& column : this->m_table.columns)
      if((column.type == mysql_column_auto_increment) && !column.default_value.is_null())
        sql << ",\n  AUTO_INCREMENT = " << column.default_value;

    POSEIDON_LOG_INFO(("Checking MySQL table:\n$1"), sql.get_string());
    const cow_vector<MySQL_Value> no_args;
    this->m_conn->execute(sql.get_string(), no_args);

    // Fetch information about existent columns and indexes.
    cow_vector<MySQL_Value> row;
    cow_vector<cow_string> fields;

    sql.clear_string();
    sql << "SHOW COLUMNS FROM `" << this->m_table.name << "`";
    this->m_conn->execute(sql.get_string(), no_args);
    this->m_conn->fetch_fields(fields);

    uint32_t idx_name = UINT32_MAX;
    uint32_t idx_type = UINT32_MAX;
    uint32_t idx_nullable = UINT32_MAX;
    uint32_t idx_default_value = UINT32_MAX;
    uint32_t idx_extra = UINT32_MAX;
    cow_dictionary<exColumn> excolumns;

    for(uint32_t t = 0;  t != fields.size();  ++t)
      if(fields[t]== "Field")
        idx_name = t;
      else if(fields[t] == "Type")
        idx_type = t;
      else if(fields[t] == "Null")
        idx_nullable = t;
      else if(fields[t] == "Default")
        idx_default_value = t;
      else if(fields[t] == "Extra")
        idx_extra = t;

    while(this->m_conn->fetch_row(row)) {
      exColumn& r = excolumns[row.at(idx_name).as_blob()];
      r.type = row.at(idx_type).as_blob();
      r.nullable = row.at(idx_nullable).as_blob() == "YES";
      r.default_value = move(row.at(idx_default_value));
      r.extra = row.at(idx_extra).as_blob();
    }

    sql.clear_string();
    sql << "SHOW INDEXES FROM `" << this->m_table.name << "`";
    this->m_conn->execute(sql.get_string(), no_args);
    this->m_conn->fetch_fields(fields);

    uint32_t idx_non_unique = UINT32_MAX;
    uint32_t idx_column_name = UINT32_MAX;
    uint32_t idx_seq_in_index = UINT32_MAX;
    cow_dictionary<exIndex> exindexes;

    for(uint32_t t = 0;  t != fields.size();  ++t)
      if(fields[t] == "Key_name")
        idx_name = t;
      else if(fields[t] == "Non_unique")
        idx_non_unique = t;
      else if(fields[t] == "Column_name")
        idx_column_name = t;
      else if(fields[t] == "Seq_in_index")
        idx_seq_in_index = t;

    while(this->m_conn->fetch_row(row)) {
      exIndex& r = exindexes[row.at(idx_name).as_blob()];
      r.multi = row.at(idx_non_unique).as_integer() != 0;
      size_t column_seq = static_cast<size_t>(row.at(idx_seq_in_index).as_integer());
      r.columns.resize(::std::max(r.columns.size(), column_seq));
      r.columns.at(column_seq - 1) = row.at(idx_column_name).as_blob();
    }

    // Compare the existent columns and indexes with the requested ones,
    // altering the table as necessary.
    sql.clear_string();
    sql << "ALTER TABLE `" << this->m_table.name << "`\n  ";
    size_t field_count = 0;

    for(const auto& column : this->m_table.columns) {
      auto ex = excolumns.find(column.name);

      if((column.type == mysql_column_dropped) && (ex == excolumns.end()))
        continue;

      if(ex != excolumns.end()) {
        switch(column.type)
          {
          case mysql_column_dropped:
            // Drop the column.
            goto do_alter_table_column_;

          case mysql_column_varchar:
            {
              // The type shall be an exact match.
              if(ex->second.type != "varchar(255)")
                goto do_alter_table_column_;

              if(column.default_value.is_null() == false) {
                // The default value is a string and shall be an exact match of
                // the configuration.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                if(ex->second.default_value.as_blob() != column.default_value.as_blob())
                  goto do_alter_table_column_;
              }

              if(ex->second.extra != "")
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_bool:
            {
              // MySQL does not have a real boolean type, so we use an integer.
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if((ex->second.type != "tinyint(1)") && (ex->second.type != "tinyint"))
                goto do_alter_table_column_;

              if(column.default_value.is_null() == false) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.get(def_value, ex->second.default_value.as_blob_data(),
                         ex->second.default_value.as_blob_size());

                if(def_value != column.default_value.as_integer())
                  goto do_alter_table_column_;
              }

              if(ex->second.extra != "")
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_int:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if((ex->second.type != "int(11)") && (ex->second.type != "int"))
                goto do_alter_table_column_;

              if(column.default_value.is_null() == false) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.get(def_value, ex->second.default_value.as_blob_data(),
                         ex->second.default_value.as_blob_size());

                if(def_value != column.default_value.as_integer())
                  goto do_alter_table_column_;
              }

              if(ex->second.extra != "")
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_int64:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if((ex->second.type != "bigint(20)") && (ex->second.type != "bigint"))
                goto do_alter_table_column_;

              if(column.default_value.is_null() == false) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.get(def_value, ex->second.default_value.as_blob_data(),
                         ex->second.default_value.as_blob_size());

                if(def_value != column.default_value.as_integer())
                  goto do_alter_table_column_;
              }

              if(ex->second.extra != "")
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_double:
            {
              // The type shall be an exact match.
              if(ex->second.type != "double")
                goto do_alter_table_column_;

              if(column.default_value.is_null() == false) {
                // The default value is a double and shall be an exact match of
                // the configuration. The MySQL server returns the default as a
                // generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                double def_value;
                ::rocket::ascii_numget numg;
                numg.get(def_value, ex->second.default_value.as_blob_data(),
                         ex->second.default_value.as_blob_size());

                if(def_value != column.default_value.as_double())
                  goto do_alter_table_column_;
              }

              if(ex->second.extra != "")
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_blob:
            {
              // The type shall be an exact match. BLOB and TEXT fields cannot
              // have a default value, so there is no need to check it.
              if(ex->second.type != "longblob")
                goto do_alter_table_column_;

              if(ex->second.extra != "")
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_datetime:
            {
              // The type shall be an exact match.
              if(ex->second.type != "datetime(6)")
                goto do_alter_table_column_;

              if(column.default_value.is_null() == false) {
                // The default value is a broken-down date/time and shall be an
                // exact match of the configuration. The MySQL server returns
                // the default as a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                char def_str[32];
                column.default_value.as_datetime().print_git_partial(def_str);
                if(::strncmp(ex->second.default_value.as_blob_data(), def_str, 19) != 0)
                  goto do_alter_table_column_;
              }

              if(ex->second.extra != "")
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_auto_increment:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              // Auto-increment fields cannot have a default value, so there is
              // no need to check it.
              if((ex->second.type != "bigint(20)") && (ex->second.type != "bigint"))
                goto do_alter_table_column_;

              if(ex->second.extra != "auto_increment")
                goto do_alter_table_column_;
            }
            break;

          default:
            POSEIDON_THROW(("Invalid MySQL column type `$1`"), column.type);
          }

        if(column.type != mysql_column_auto_increment) {
          // An auto-increment field is always non-null and has no default value
          // so these hardly make any sense.
          if(ex->second.nullable != column.nullable)
            goto do_alter_table_column_;

          if(ex->second.default_value.is_null() != column.default_value.is_null())
            goto do_alter_table_column_;
        }

        POSEIDON_LOG_DEBUG(("Verified column `$1.$2`"), this->m_table.name, column.name);
        continue;
      }

  do_alter_table_column_:
      // The column does not match the definition, so modify it.
      if(++ field_count != 1)
        sql << ",\n  ";

      if(ex == excolumns.end()) {
        sql << "ADD COLUMN ";
        do_append_column_definition(sql, column);
      }
      else if(column.type != mysql_column_dropped) {
        sql << "MODIFY COLUMN ";
        do_append_column_definition(sql, column);
      }
      else
        sql << "DROP COLUMN `" << column.name << "`";
    }

    for(const auto& index : this->m_table.indexes) {
      auto ex = exindexes.find(index.name);

      if((index.type == mysql_index_dropped) && (ex == exindexes.end()))
        continue;

      if(ex != exindexes.end()) {
        switch(index.type)
          {
          case mysql_index_dropped:
            // Drop the index.
            goto do_alter_table_index_;

          case mysql_index_unique:
            {
              if(ex->second.multi)
                goto do_alter_table_index_;
            }
            break;

          case mysql_index_multi:
            {
              if(!ex->second.multi)
                goto do_alter_table_index_;
            }
            break;

          default:
            POSEIDON_THROW(("Invalid MySQL index type `$1`"), index.type);
          }

        if(ex->second.columns.size() != index.columns.size())
          goto do_alter_table_index_;

        for(uint32_t t = 0;  t != index.columns.size();  ++t)
          if(ex->second.columns[t] != index.columns[t])
            goto do_alter_table_index_;

        POSEIDON_LOG_DEBUG(("Verified index `$1.$2`"), this->m_table.name, index.name);
        continue;
      }

  do_alter_table_index_:
      // The index does not match the definition, so modify it.
      if(++ field_count != 1)
        sql << ",\n  ";

      if(ex == exindexes.end()) {
        sql << "ADD ";
        do_append_index_definition(sql, index);
      }
      else {
        if(index.name == "PRIMARY")
          sql << "DROP PRIMARY KEY";
        else
          sql << "DROP INDEX `" << index.name << "`";

        if(index.type != mysql_index_dropped) {
          sql << ",\n   ADD ";
          do_append_index_definition(sql, index);
        }
      }
    }

    if(field_count == 0)
      return;

    POSEIDON_LOG_WARN(("Updating MySQL table structure:", "$1"), sql.get_string());
    this->m_conn->execute(sql.get_string(), no_args);
    this->m_altered = true;
  }

void
MySQL_Check_Table_Future::
do_on_abstract_future_finalize()
  {
    if(!this->m_conn)
      return;

    if(this->m_conn->reset())
      this->m_ctr->pool_connection(move(this->m_conn));
  }

void
MySQL_Check_Table_Future::
do_on_abstract_task_execute()
  {
    this->do_abstract_future_initialize_once();
  }

}  // namespace poseidon
