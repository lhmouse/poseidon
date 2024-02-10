// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_ENUMS_
#define POSEIDON_MYSQL_ENUMS_

#include "../fwd.hpp"
#include "enums.hpp"
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

}  // namespace poseidon
#endif
