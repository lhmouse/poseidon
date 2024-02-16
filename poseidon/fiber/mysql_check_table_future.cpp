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
        sql << "datetime";
        break;

      case mysql_column_auto_increment:
        sql << "bigint AUTO_INCREMENT";
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL column type `$1`"), column.type);
      }

    if(column.nullable == false)
      sql << " NOT NULL";

    if(!column.default_value.is_null())
      sql << " DEFAULT " << column.default_value;
  }

void
do_append_index_definition(tinyfmt_str& sql, const MySQL_Table_Structure::Index& index)
  {
    if(ascii_ci_equal(index.name, "PRIMARY"))
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
    vector<cow_string> columns;
  };

}  // namespace

MySQL_Check_Table_Future::
MySQL_Check_Table_Future(MySQL_Connector& connector, MySQL_Table_Structure table)
  {
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
    if(table_name.empty())
      POSEIDON_THROW(("Table has no name"));

    tinyfmt_str sql;
    sql << "CREATE TABLE IF NOT EXISTS `" << table_name << "`\n  (";
    size_t ncolumns = 0;
    size_t nindexes = 0;

    for(const auto& column : this->m_res.table.columns())
      if(column.type != mysql_column_dropped) {
        if(++ ncolumns != 1)
          sql << ",\n   ";
        do_append_column_definition(sql, column);
      }

    if(ncolumns == 0)
      POSEIDON_THROW(("Table `$1` contains no column"), table_name);

    for(const auto& index : this->m_res.table.indexes())
      if(index.type != mysql_index_dropped) {
        ++ nindexes;
        sql << ",\n   ";
        do_append_index_definition(sql, index);
      }

    sql << ")\n  ";
    do_append_engine(sql, this->m_res.table.engine());
    sql << ",\n  CHARSET = 'utf8mb4'";

    POSEIDON_LOG_INFO(("Checking MySQL table:\n$1"), sql.get_string());
    conn->execute(sql.get_string(), nullptr, 0);
    this->m_res.warning_count = conn->warning_count();
    this->m_res.altered = false;

    // Fetch information about existent columns and indexes.
    vector<cow_string> fields;
    vector<MySQL_Value> row;

    sql.clear_string();
    sql << "SHOW COLUMNS FROM `" << table_name << "`";
    conn->execute(sql.get_string(), nullptr, 0);
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
      cow_string& name = row.at(ix_name.value()).mut_blob();
      if(auto column_config = this->m_res.table.find_column_opt(name))
        name = column_config->name;

      // Save this column.
      exColumn& r = excolumns[name];
      r.type = row.at(ix_type.value()).as_blob();
      r.nullable = ascii_ci_equal(row.at(ix_nullable.value()).as_blob(), "yes");
      r.default_value = move(row.at(ix_default_value.value()));
      r.extra = row.at(ix_extra.value()).as_blob();
    }

    sql.clear_string();
    sql << "SHOW INDEXES FROM `" << table_name << "`";
    conn->execute(sql.get_string(), nullptr, 0);
    conn->fetch_fields(fields);

    map<cow_string, exIndex> exindexes;
    opt<uint32_t> ix_non_unique, ix_column_name, ix_seq_in_index;

    for(uint32_t t = 0;  t != fields.size();  ++t)
      if(ascii_ci_equal(fields[t], "key_name"))
        ix_name = t;
      else if(ascii_ci_equal(fields[t], "non_unique"))
        ix_non_unique = t;
      else if(ascii_ci_equal(fields[t], "column_name"))
        ix_column_name = t;
      else if(ascii_ci_equal(fields[t], "seq_in_index"))
        ix_seq_in_index = t;

    while(conn->fetch_row(row)) {
      cow_string& name = row.at(ix_name.value()).mut_blob();
      if(auto index_config = this->m_res.table.find_index_opt(name))
        name = index_config->name;

      // Save this index.
      exIndex& r = exindexes[name];
      r.multi = row.at(ix_non_unique.value()).as_integer() != 0;
      size_t column_seq = static_cast<size_t>(row.at(ix_seq_in_index.value()).as_integer());
      r.columns.resize(::std::max(r.columns.size(), column_seq));
      r.columns.at(column_seq - 1) = row.at(ix_column_name.value()).as_blob();
    }

    // Compare the existent columns and indexes with the requested ones,
    // altering the table as necessary.
    sql.clear_string();
    sql << "ALTER TABLE `" << table_name << "`\n  ";
    size_t nalters = 0;

    for(const auto& column : this->m_res.table.columns()) {
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
              if(!ascii_ci_equal(ex->second.type, "varchar(255)"))
                goto do_alter_table_column_;

              if(!column.default_value.is_null()) {
                // The default value is a string and shall be an exact match of
                // the configuration.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                if(ex->second.default_value.as_blob() != column.default_value.as_blob())
                  goto do_alter_table_column_;
              }

              if(!ascii_ci_equal(ex->second.extra, ""))
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_bool:
            {
              // MySQL does not have a real boolean type, so we use an integer.
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if(!ascii_ci_equal(ex->second.type, "tinyint(1)") && !ascii_ci_equal(ex->second.type, "tinyint"))
                goto do_alter_table_column_;

              if(!column.default_value.is_null()) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DI(ex->second.default_value.blob_data(), ex->second.default_value.blob_size());
                numg.cast_I(def_value, INT64_MIN, INT64_MAX);

                if(def_value != column.default_value.as_integer())
                  goto do_alter_table_column_;
              }

              if(!ascii_ci_equal(ex->second.extra, ""))
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_int:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if(!ascii_ci_equal(ex->second.type, "int(11)") && !ascii_ci_equal(ex->second.type, "int"))
                goto do_alter_table_column_;

              if(!column.default_value.is_null()) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DI(ex->second.default_value.blob_data(), ex->second.default_value.blob_size());
                numg.cast_I(def_value, INT64_MIN, INT64_MAX);

                if(def_value != column.default_value.as_integer())
                  goto do_alter_table_column_;
              }

              if(!ascii_ci_equal(ex->second.extra, ""))
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_int64:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              if(!ascii_ci_equal(ex->second.type, "bigint(20)") && !ascii_ci_equal(ex->second.type, "bigint"))
                goto do_alter_table_column_;

              if(!column.default_value.is_null()) {
                // The default value is an integer and shall be an exact match
                // of the configuration. The MySQL server returns the default as
                // a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                int64_t def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DI(ex->second.default_value.blob_data(), ex->second.default_value.blob_size());
                numg.cast_I(def_value, INT64_MIN, INT64_MAX);

                if(def_value != column.default_value.as_integer())
                  goto do_alter_table_column_;
              }

              if(!ascii_ci_equal(ex->second.extra, ""))
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_double:
            {
              // The type shall be an exact match.
              if(!ascii_ci_equal(ex->second.type, "double"))
                goto do_alter_table_column_;

              if(!column.default_value.is_null()) {
                // The default value is a double and shall be an exact match of
                // the configuration. The MySQL server returns the default as a
                // generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                double def_value;
                ::rocket::ascii_numget numg;
                numg.parse_DD(ex->second.default_value.blob_data(), ex->second.default_value.blob_size());
                numg.cast_D(def_value, -HUGE_VAL, HUGE_VAL);

                if(def_value != column.default_value.as_double())
                  goto do_alter_table_column_;
              }

              if(!ascii_ci_equal(ex->second.extra, ""))
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_blob:
            {
              // The type shall be an exact match. BLOB and TEXT fields cannot
              // have a default value, so there is no need to check it.
              if(!ascii_ci_equal(ex->second.type, "longblob"))
                goto do_alter_table_column_;

              if(!ascii_ci_equal(ex->second.extra, ""))
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_datetime:
            {
              // The type shall be an exact match.
              if(!ascii_ci_equal(ex->second.type, "datetime"))
                goto do_alter_table_column_;

              if(!column.default_value.is_null()) {
                // The default value is a broken-down date/time and shall be an
                // exact match of the configuration. The MySQL server returns
                // the default as a generic string, so parse it first.
                if(!ex->second.default_value.is_blob())
                  goto do_alter_table_column_;

                char def_str[32];
                column.default_value.as_datetime().print_iso8601_partial(def_str);
                ::memcpy(def_str + 19, ".000000", 8);
                size_t cmp_len = ex->second.default_value.blob_size();

                if((cmp_len < 19) || (cmp_len > 26))
                  goto do_alter_table_column_;

                if(::memcmp(ex->second.default_value.blob_data(), def_str, cmp_len) != 0)
                  goto do_alter_table_column_;
              }

              if(!ascii_ci_equal(ex->second.extra, ""))
                goto do_alter_table_column_;
            }
            break;

          case mysql_column_auto_increment:
            {
              // Some old MySQL versions include a display width in the type,
              // which is deprecated since 8.0 anyway. Both forms are accepted.
              // Auto-increment fields cannot have a default value, so there is
              // no need to check it.
              if(!ascii_ci_equal(ex->second.type, "bigint(20)") && !ascii_ci_equal(ex->second.type, "bigint"))
                goto do_alter_table_column_;

              if(!ascii_ci_equal(ex->second.extra, "AUTO_INCREMENT"))
                goto do_alter_table_column_;
            }
            break;

          default:
            POSEIDON_THROW(("Invalid MySQL column type `$1`"), column.type);
          }

        if(ex->second.nullable != column.nullable)
          goto do_alter_table_column_;

        if(ex->second.default_value.is_null() != column.default_value.is_null())
          goto do_alter_table_column_;

        POSEIDON_LOG_DEBUG(("Verified: table `$1` column `$2`"), table_name, column.name);
        continue;
      }

  do_alter_table_column_:
      // The column does not match the definition, so modify it.
      if(++ nalters != 1)
        sql << ",\n  ";

      if(ex == excolumns.end()) {
        sql << "ADD COLUMN";
        do_append_column_definition(sql, column);
      }
      else if(column.type != mysql_column_dropped) {
        sql << "MODIFY COLUMN";
        do_append_column_definition(sql, column);
      }
      else
        sql << "DROP COLUMN `" << column.name << "`";
    }

    for(const auto& index : this->m_res.table.indexes()) {
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

        POSEIDON_LOG_DEBUG(("Verified: table `$1` index `$2`"), table_name, index.name);
        continue;
      }

  do_alter_table_index_:
      // The index does not match the definition, so modify it.
      if(++ nalters != 1)
        sql << ",\n  ";

      if(ex == exindexes.end()) {
        sql << "ADD";
        do_append_index_definition(sql, index);
      }
      else {
        if(ascii_ci_equal(index.name, "PRIMARY"))
          sql << "DROP PRIMARY KEY";
        else
          sql << "DROP INDEX `" << index.name << "`";

        if(index.type != mysql_index_dropped) {
          sql << ",\n   ADD";
          do_append_index_definition(sql, index);
        }
      }
    }

    if(nalters == 0)
      return;

    POSEIDON_LOG_WARN(("Updating MySQL table structure:", "$1"), sql.get_string());
    conn->execute(sql.get_string(), nullptr, 0);
    this->m_res.altered = true;
  }

void
MySQL_Check_Table_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
