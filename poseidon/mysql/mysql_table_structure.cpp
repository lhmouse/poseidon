// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_table_structure.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

bool
do_is_name_valid(cow_stringR name)
  {
    return (name.size() != 0)
        && ::rocket::all_of(name,
             [](char c) {
               return ((c >= '0') && (c <= '9'))
                      || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                      || (c == '_') || (c == '$') || (c == '-') || (c == '~');
             });
  }

template<typename elementT>
size_t
do_add_element(cow_vector<elementT>& container, const elementT& element)
  {
    // Find and update.
    for(size_t k = 0;  k != container.size();  ++k)
      if(ascii_ci_equal(container[k].name, element.name)) {
        container.mut(k) = element;
        return k;
      }

    // Append a new one.
    container.emplace_back(element);
    return container.size() - 1;
  }

}  // namespace

MySQL_Table_Structure::
~MySQL_Table_Structure()
  {
  }

void
MySQL_Table_Structure::
set_name(cow_stringR name)
  {
    if(!do_is_name_valid(name))
      POSEIDON_THROW(("Invalid MySQL table name `$1`"), name);

    this->m_name = name;
  }

void
MySQL_Table_Structure::
set_engine(MySQL_Engine_Type engine)
  {
    switch(engine) {
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

const MySQL_Table_Structure::Column*
MySQL_Table_Structure::
find_column_opt(cow_stringR name) const noexcept
  {
    return ::rocket::find_if(this->m_columns,
             [&](const Column& r) { return ascii_ci_equal(r.name, name);  });
  }

size_t
MySQL_Table_Structure::
add_column(const Column& column)
  {
    if(!do_is_name_valid(column.name))
      POSEIDON_THROW(("Invalid MySQL column name `$1`"), column.name);

    switch(column.type) {
    case mysql_column_varchar:
      {
        if(column.default_value.is_null())
          break;

        if(!column.default_value.is_string())
          POSEIDON_THROW((
              "Invalid default value for column `$1`"),
              column.name);

        const cow_string& val = column.default_value.as_string();

        size_t offset = 0, count = 0;;
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

        if((val != 0) && (val != 1))
          POSEIDON_THROW((
              "Default value for column `$1` is out of range"),
              column.name);
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

        if((val < INT32_MIN) && (val > INT32_MAX))
          POSEIDON_THROW((
              "Default value for column `$1` is out of range"),
              column.name);
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

        if(!column.default_value.is_mysql_time())
          POSEIDON_THROW((
              "Invalid default value for column `$1`"),
              column.name);

        const ::MYSQL_TIME& val = column.default_value.as_mysql_time();

        if(val.time_type != MYSQL_TIMESTAMP_DATETIME)
          POSEIDON_THROW((
              "Default value for column `$1` must be `MYSQL_TIMESTAMP_DATETIME`"),
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
              "Auto-increment column `$1` is not nullable"),
              column.name);

        for(const auto& other_column : this->m_columns)
          if(other_column.type == mysql_column_auto_increment)
            POSEIDON_THROW((
                "Another auto-increment column `$1` already exists"),
                other_column.name);
      }
      break;

    default:
      POSEIDON_THROW(("Invalid MySQL data type `$1`"), column.type);
    }

    return do_add_element(this->m_columns, column);
  }

const MySQL_Table_Structure::Index*
MySQL_Table_Structure::
find_index_opt(cow_stringR name) const noexcept
  {
    return ::rocket::find_if(this->m_indexes,
             [&](const Index& r) { return ascii_ci_equal(r.name, name);  });
  }

size_t
MySQL_Table_Structure::
add_index(const Index& index)
  {
    if(!do_is_name_valid(index.name))
      POSEIDON_THROW(("Invalid MySQL index name `$1`"), index.name);

    if(index.columns.empty())
      POSEIDON_THROW(("MySQL index `$1` contains no column"), index.name);

    bool primary = ascii_ci_equal(index.name, "PRIMARY");
    if(primary && !index.unique)
      POSEIDON_THROW(("Primary key must also be unique"));

    for(size_t icol = 0;  icol != index.columns.size();  ++icol) {
      // Verify this column.
      auto col = this->find_column_opt(index.columns[icol]);
      if(!col)
        POSEIDON_THROW((
            "MySQL index `$1` contains non-existent column `$2`"),
            index.name, index.columns[icol]);

      if(primary && col->nullable)
        POSEIDON_THROW((
            "Primary key shall not contain nullable column `$1`"),
            index.columns[icol]);

      for(size_t t = 0;  t != icol;  ++t)
        if(ascii_ci_equal(index.columns[t], index.columns[icol]))
          POSEIDON_THROW((
              "MySQL index `$1` contains duplicate column `$2`"),
              index.name, index.columns[icol]);
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
