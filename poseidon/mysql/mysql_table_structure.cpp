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
      && all_of(name,
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

size_t
MySQL_Table_Structure::
add_column(const Column& column)
  {
    if(!do_is_name_valid(column.name))
      POSEIDON_THROW(("Invalid MySQL column name `$1`"), column.name);

    switch(column.type) {
      case mysql_column_varchar:
      case mysql_column_bool:
      case mysql_column_int:
      case mysql_column_int64:
      case mysql_column_double:
      case mysql_column_blob:
      case mysql_column_datetime:
      case mysql_column_auto_increment:
        break;

      default:
        POSEIDON_THROW(("Invalid MySQL data type `$1`"), column.type);
    }

    return do_add_element(this->m_columns, column);
  }

size_t
MySQL_Table_Structure::
add_index(const Index& index)
  {
    if(!do_is_name_valid(index.name))
      POSEIDON_THROW(("Invalid MySQL index name `$1`"), index.name);

    if(!index.unique && ascii_ci_equal(index.name, "PRIMARY"))
      POSEIDON_THROW(("Primary indexes must also be unique"));

    if(index.columns.empty())
      POSEIDON_THROW(("MySQL index `$1` contains no column"), index.name);

    for(const auto& index_column : index.columns)
      if(none_of(this->m_columns,
           [&](const Column& my_column) {
             return ascii_ci_equal(my_column.name, index_column);
           }))
        POSEIDON_THROW(("MySQL index `$1` contains non-existent column `$2`"),
                       index.name, index_column);

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
