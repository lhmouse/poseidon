// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_table_structure.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

bool
do_is_name_valid(const cow_string& name)
  {
    return (name.size() != 0)
        && ::rocket::all_of(name,
             [](char c) {
               return ((c >= '0') && (c <= '9'))
                      || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                      || (c == '_') || (c == '$') || (c == '-') || (c == '~');
             });
  }

bool
do_names_equal(const cow_string& lhs, const cow_string& rhs)
  {
    return ::rocket::ascii_ci_equal(lhs.data(), lhs.size(), rhs.data(), rhs.size());
  }

template<typename elementT>
size_t
do_add_element(cow_vector<elementT>& container, const elementT& element)
  {
    // Find and update.
    for(size_t k = 0;  k != container.size();  ++k)
      if(do_names_equal(container[k].name, element.name)) {
        container.mut(k) = element;
        return k;
      }

    // Append a new one.
    container.emplace_back(element);
    return container.size() - 1;
  }

}  // namespace

MySQL_Table_Structure::
MySQL_Table_Structure(const cow_string& name, MySQL_Engine_Type engine)
  {
    this->m_name = name;
    this->m_engine = engine;
  }

MySQL_Table_Structure::
~MySQL_Table_Structure()
  {
  }

void
MySQL_Table_Structure::
set_name(const cow_string& name)
  {
    if(!do_is_name_valid(name))
      POSEIDON_THROW(("Invalid MySQL table name `$1`"), name);

    this->m_name = name;
  }

void
MySQL_Table_Structure::
set_engine(MySQL_Engine_Type engine)
  {
    switch(engine)
      {
      case mysql_engine_innodb:
      case mysql_engine_myisam:
      case mysql_engine_memory:
      case mysql_engine_archive:
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL storage engine `$1`"), engine);
      }

    this->m_engine = engine;
  }

const MySQL_Table_Column*
MySQL_Table_Structure::
find_column_opt(const cow_string& name) const noexcept
  {
    return ::rocket::find_if(this->m_columns,
        [&](const MySQL_Table_Column& r) { return do_names_equal(name, r.name);  });
  }

size_t
MySQL_Table_Structure::
add_column(const MySQL_Table_Column& column)
  {
    if(!do_is_name_valid(column.name))
      POSEIDON_THROW(("Invalid MySQL column name `$1`"), column.name);

    switch(column.type)
      {
      case mysql_column_dropped:
        {
          if(!column.default_value.is_null())
            POSEIDON_THROW((
                "Dropped column `$1` shall not have a default value"),
                column.name);

          // Drop all indexes that contain this column.
          for(size_t t = 0;  t != this->m_indexes.size();  ++t)
            for(const auto& col_name : this->m_indexes[t].columns)
              if(col_name == column.name) {
                this->m_indexes.mut(t).type = mysql_index_dropped;
                break;
              }
        }
        break;

      case mysql_column_varchar:
        {
          if(column.default_value.is_null())
            break;

          if(!column.default_value.is_blob())
            POSEIDON_THROW((
                "Invalid default value for column `$1`"),
                column.name);

          const cow_string& val = column.default_value.as_blob();

          // Verify the default value. MySQL store a VARCHAR value as a series
          // of UTF characters, so count them.
          size_t offset = 0, count = 0;
          char32_t cp;
          while(::asteria::utf8_decode(cp, val, offset))
            count ++;

          if(count > 255)
            POSEIDON_THROW((
                "Default string for column `$1` is too long"),
                column.name);
        }
        break;

      case mysql_column_bool:
        {
          if(column.default_value.is_null())
            break;

          if(!column.default_value.is_integer())
            POSEIDON_THROW((
                "Invalid default value for column `$1`"),
                column.name);

          const int64_t val = column.default_value.as_integer();

          // Verify the default value, which shall be an integer of zero for
          // `false` and one for `true`.
          if((val != 0) && (val != 1))
            POSEIDON_THROW((
                "Default value `$2` for column `$1` is out of range"),
                column.name, val);
        }
        break;

      case mysql_column_int:
        {
          if(column.default_value.is_null())
            break;

          if(!column.default_value.is_integer())
            POSEIDON_THROW((
                "Invalid default value for column `$1`"),
                column.name);

          const int64_t val = column.default_value.as_integer();

          // Verify the default value, which shall be a 32-bit signed integer.
          if((val < INT32_MIN) && (val > INT32_MAX))
            POSEIDON_THROW((
                "Default value `$2` for column `$1` is out of range"),
                column.name, val);
        }
        break;

      case mysql_column_int64:
        {
          if(column.default_value.is_null())
            break;

          if(!column.default_value.is_integer())
            POSEIDON_THROW((
                "Invalid default value for column `$1`"),
                column.name);
        }
        break;

      case mysql_column_double:
        {
          if(column.default_value.is_null())
            break;

          if(!column.default_value.is_double())
            POSEIDON_THROW((
                "Invalid default value for column `$1`"),
                column.name);

          const double val = column.default_value.as_double();

          // Verify the default value, which shall be a finite double value.
          // MySQL does not support infinities or NaNs.
          if(!::std::isfinite(val))
            POSEIDON_THROW((
                "Default value for column `$1` shall be finite"),
                column.name);
        }
        break;

      case mysql_column_blob:
        {
          if(!column.default_value.is_null())
            POSEIDON_THROW((
                "BLOB column `$1` shall not have a default value"),
                column.name);
        }
        break;

      case mysql_column_datetime:
        {
          if(column.default_value.is_null())
            break;

          if(!column.default_value.is_datetime())
            POSEIDON_THROW((
                "Invalid default value for column `$1`"),
                column.name);

          struct timespec ts;
          timespec_from_system_time(ts, column.default_value.as_system_time());
          const system_time val = system_clock::from_time_t(ts.tv_sec);

          // Verify the default value, which shall be a timestamp between
          // '1900-01-01 00:00:00 UTC' and '9999-12-31 23:59:59 UTC', with no
          // fractional part.
          if((ts.tv_sec < -2208988800) || (ts.tv_sec > 253402300799))
            POSEIDON_THROW((
                "Default value for column `$1` is out of range"),
                column.name);

          if(val != column.default_value.as_system_time())
            POSEIDON_THROW((
                "Default value for column `$1` shall not contain a fraction"),
                column.name);
        }
        break;

      case mysql_column_auto_increment:
        {
          if(!column.default_value.is_null())
            POSEIDON_THROW((
                "Auto-increment column `$1` shall not have a default value"),
                column.name);

          if(column.nullable)
            POSEIDON_THROW((
                "Auto-increment column `$1` shall not be nullable"),
                column.name);

          // There shall be no more than one auto-increment column.
          for(const auto& other : this->m_columns)
            if((other.type == mysql_column_auto_increment) && (other.name != column.name))
              POSEIDON_THROW((
                  "Another auto-increment column `$1` already exists"),
                  other.name);
        }
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL column type `$1`"), column.type);
      }

    return do_add_element(this->m_columns, column);
  }

const MySQL_Table_Index*
MySQL_Table_Structure::
find_index_opt(const cow_string& name) const noexcept
  {
    return ::rocket::find_if(this->m_indexes,
        [&](const MySQL_Table_Index& r) { return do_names_equal(name, r.name);  });
  }

size_t
MySQL_Table_Structure::
add_index(const MySQL_Table_Index& index)
  {
    if(!do_is_name_valid(index.name))
      POSEIDON_THROW(("Invalid MySQL index name `$1`"), index.name);

    switch(index.type)
      {
      case mysql_index_dropped:
        {
          if(!index.columns.empty())
            POSEIDON_THROW((
                "Dropped index `$1` shall comprise no column"),
                index.name);
        }
        break;

      case mysql_index_unique:
        {
          if(index.columns.empty())
            POSEIDON_THROW((
                "Unique index `$1` shall comprise at least one column"),
                index.name);
        }
        break;

      case mysql_index_multi:
        {
          if(index.columns.empty())
            POSEIDON_THROW((
                "Non-unique index `$1` shall comprise at least one column"),
                index.name);

          if(do_names_equal(index.name, &"PRIMARY"))
            POSEIDON_THROW((
                "A primary key must be unique"));
        }
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL index type `$1`"), index.type);
      }

    // Check all columns.
    for(const auto& col_name : index.columns) {
      auto col = this->find_column_opt(col_name);
      if(!col)
        POSEIDON_THROW((
            "Index `$1` references non-existent column `$2`"),
            index.name, col_name);

      if(col->nullable && do_names_equal(index.name, &"PRIMARY"))
        POSEIDON_THROW((
            "Primary key shall not contain nullable column `$1`"),
            col_name);

      for(const auto& other : index.columns)
        if((&other < &col_name) && (other == col_name))
          POSEIDON_THROW((
              "Index `$1` contains duplicate column `$2`"),
              index.name, col_name);
    }

    return do_add_element(this->m_indexes, index);
  }

void
MySQL_Table_Structure::
clear_columns() noexcept
  {
    this->m_columns.clear();
    this->m_indexes.clear();
  }

}  // namespace poseidon
