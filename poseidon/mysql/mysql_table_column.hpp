// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_TABLE_COLUMN_
#define POSEIDON_MYSQL_MYSQL_TABLE_COLUMN_

#include "../fwd.hpp"
#include "enums.hpp"
#include "mysql_value.hpp"
#include "../http/http_field_name.hpp"
namespace poseidon {

struct MySQL_Table_Column
  {
    HTTP_Field_Name name;
    MySQL_Column_Type type = mysql_column_varchar;
    bool nullable = false;
    MySQL_Value default_value;

    MySQL_Table_Column() noexcept = default;
    MySQL_Table_Column(const MySQL_Table_Column&) = default;
    MySQL_Table_Column(MySQL_Table_Column&&) = default;
    MySQL_Table_Column& operator=(const MySQL_Table_Column&) & = default;
    MySQL_Table_Column& operator=(MySQL_Table_Column&&) & = default;
    ~MySQL_Table_Column();

    MySQL_Table_Column&
    swap(MySQL_Table_Column& other) noexcept
      {
        this->name.swap(other.name);
        ::std::swap(this->type, other.type);
        ::std::swap(this->nullable, other.nullable);
        this->default_value.swap(other.default_value);
        return *this;
      }
  };

inline
void
swap(MySQL_Table_Column& lhs, MySQL_Table_Column& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
