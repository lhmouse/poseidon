// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_TABLE_STRUCTURE_
#define POSEIDON_MYSQL_MYSQL_TABLE_STRUCTURE_

#include "../fwd.hpp"
#include "enums.hpp"
#include "mysql_table_column.hpp"
#include "mysql_table_index.hpp"
namespace poseidon {

class MySQL_Table_Structure
  {
  private:
    cow_string m_name;
    MySQL_Engine_Type m_engine = mysql_engine_innodb;
    cow_vector<MySQL_Table_Column> m_columns;
    cow_vector<MySQL_Table_Index> m_indexes;

  public:
    // Constructs an empty table.
    constexpr
    MySQL_Table_Structure() noexcept = default;

    MySQL_Table_Structure&
    swap(MySQL_Table_Structure& other) noexcept
      {
        this->m_name.swap(other.m_name);
        this->m_columns.swap(other.m_columns);
        this->m_indexes.swap(other.m_indexes);
        ::std::swap(this->m_engine, other.m_engine);
        return *this;
      }

  public:
    MySQL_Table_Structure(const MySQL_Table_Structure&) noexcept = default;
    MySQL_Table_Structure(MySQL_Table_Structure&&) noexcept = default;
    MySQL_Table_Structure& operator=(const MySQL_Table_Structure&) & noexcept = default;
    MySQL_Table_Structure& operator=(MySQL_Table_Structure&&) & noexcept = default;
    ~MySQL_Table_Structure();

    // Gets and sets the table name.
    // If the table name is not a valid identifier, an exception is thrown, and
    // there is no effect.
    const cow_string&
    name() const noexcept
      { return this->m_name;  }

    void
    set_name(const cow_string& name);

    // Gets and sets the storage engine.
    // If the storage engine is not valid, an exception is thrown, and there is
    // no effect.
    MySQL_Engine_Type
    engine() const noexcept
      { return this->m_engine;  }

    void
    set_engine(MySQL_Engine_Type engine);

    // Gets all columns.
    const cow_vector<MySQL_Table_Column>&
    columns() const noexcept
      { return this->m_columns;  }

    // Gets a column.
    size_t
    count_columns() const noexcept
      { return this->m_columns.size();  }

    const MySQL_Table_Column&
    column(size_t pos) const
      { return this->m_columns.at(pos);  }

    const MySQL_Table_Column*
    find_column_opt(const cow_string& name) const noexcept;

    // Adds a column. If a column with the same name already exists, it will be
    // updated in place. Setting a column to `mysql_column_dropped` automatically
    // drops all indexes which contain the column.
    // Returns the  subscript of the updated column.
    // If `column` references an invalid column, such as an invalid name or type,
    // an exception is thrown, and there is no effect.
    size_t
    add_column(const MySQL_Table_Column& column);

    // Gets all indexes.
    const cow_vector<MySQL_Table_Index>&
    indexes() const noexcept
      { return this->m_indexes;  }

    // Gets an index.
    size_t
    count_indexes() const noexcept
      { return this->m_indexes.size();  }

    const MySQL_Table_Index&
    index(size_t pos) const
      { return this->m_indexes.at(pos);  }

    const MySQL_Table_Index*
    find_index_opt(const cow_string& name) const noexcept;

    // Adds an index. If an index with the same name already exists, it will be
    // updated in place. If `index.name` equals `"primary"`, it denotes the
    // primary index, and `index.unique` must be `true`.
    // Returns the  subscript of the updated index.
    // If `index` references an invalid index, such as an invalid name or a
    // non-existent column, an exception is thrown, and there is no effect.
    size_t
    add_index(const MySQL_Table_Index& index);

    // Clears all columns with all indexes.
    void
    clear_columns() noexcept;
  };

inline
void
swap(MySQL_Table_Structure& lhs, MySQL_Table_Structure& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
