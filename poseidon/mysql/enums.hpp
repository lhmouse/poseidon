// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_ENUMS_
#define POSEIDON_MYSQL_ENUMS_

#include "../fwd.hpp"
namespace poseidon {

enum MySQL_Column_Type : uint8_t
  {
    mysql_column_dropped         = 0,  // [deleted]
    mysql_column_varchar         = 1,  // varchar(255)
    mysql_column_bool            = 2,  // tinyint
    mysql_column_int             = 3,  // int
    mysql_column_int64           = 4,  // bigint
    mysql_column_double          = 5,  // double
    mysql_column_blob            = 6,  // longblob
    mysql_column_datetime        = 7,  // datetime
    mysql_column_auto_increment  = 8,  // bigint auto_increment
  };

enum MySQL_Index_Type : uint8_t
  {
    mysql_index_dropped   = 0,  // [deleted]
    mysql_index_unique    = 1,  // unique index
    mysql_index_multi     = 2,  // index
  };

enum MySQL_Engine_Type : uint8_t
  {
    mysql_engine_innodb   = 0,
    mysql_engine_myisam   = 1,
    mysql_engine_memory   = 2,
    mysql_engine_archive  = 3,
  };

enum MySQL_Value_Type : uint8_t
  {
    mysql_value_null       = 0,  // NULL
    mysql_value_integer    = 1,  // tinyint, int, bigint
    mysql_value_double     = 2,  // double
    mysql_value_blob       = 3,  // varchar(255), longblob
    mysql_value_datetime   = 4,  // datetime
  };

}  // namespace poseidon
#endif
