// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_TABLE_STRUCTURE_
#define POSEIDON_MYSQL_MYSQL_TABLE_STRUCTURE_

#include "../fwd.hpp"
#include "enums.hpp"
#include "mysql_table_column.hpp"
#include "mysql_table_index.hpp"
namespace poseidon {

struct MySQL_Table_Structure
  {
    cow_string name;
    MySQL_Engine_Type engine = mysql_engine_innodb;
    cow_vector<MySQL_Table_Column> columns;
    cow_vector<MySQL_Table_Index> indexes;

    MySQL_Table_Structure() noexcept = default;
    MySQL_Table_Structure(const MySQL_Table_Structure&) = default;
    MySQL_Table_Structure(MySQL_Table_Structure&&) = default;
    MySQL_Table_Structure& operator=(const MySQL_Table_Structure&) & = default;
    MySQL_Table_Structure& operator=(MySQL_Table_Structure&&) & = default;
    ~MySQL_Table_Structure();

    MySQL_Table_Structure&
    swap(MySQL_Table_Structure& other)
      noexcept
      {
        this->name.swap(other.name);
        this->columns.swap(other.columns);
        this->indexes.swap(other.indexes);
        ::std::swap(this->engine, other.engine);
        return *this;
      }

    void
    clear()
      noexcept
      {
        this->name.clear();
        this->engine = mysql_engine_innodb;
        this->columns.clear();
        this->indexes.clear();
      }
  };

inline
void
swap(MySQL_Table_Structure& lhs, MySQL_Table_Structure& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
