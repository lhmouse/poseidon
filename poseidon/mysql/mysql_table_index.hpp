// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_TABLE_INDEX_
#define POSEIDON_MYSQL_MYSQL_TABLE_INDEX_

#include "../fwd.hpp"
#include "enums.hpp"
#include "mysql_value.hpp"
#include "../http/http_field_name.hpp"
namespace poseidon {

struct MySQL_Table_Index
  {
    HTTP_Field_Name name;
    MySQL_Index_Type type = mysql_index_multi;
    cow_vector<HTTP_Field_Name> columns;

    MySQL_Table_Index() noexcept = default;
    MySQL_Table_Index(const MySQL_Table_Index&) = default;
    MySQL_Table_Index(MySQL_Table_Index&&) = default;
    MySQL_Table_Index& operator=(const MySQL_Table_Index&) & = default;
    MySQL_Table_Index& operator=(MySQL_Table_Index&&) & = default;
    ~MySQL_Table_Index();

    MySQL_Table_Index&
    swap(MySQL_Table_Index& other) noexcept
      {
        this->name.swap(other.name);
        ::std::swap(this->type, other.type);
        this->columns.swap(other.columns);
        return *this;
      }
  };

inline
void
swap(MySQL_Table_Index& lhs, MySQL_Table_Index& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
