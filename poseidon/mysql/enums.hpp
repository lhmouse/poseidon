// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_ENUMS_
#define POSEIDON_MYSQL_ENUMS_

#include "../fwd.hpp"
namespace poseidon {

enum MySQL_Column_Type : uint8_t
  {
    mysql_column_varchar         = 0,  // varchar(255)
    mysql_column_bool            = 1,  // tinyint(1)
    mysql_column_int             = 2,  // int(11)
    mysql_column_int64           = 3,  // bigint(20)
    mysql_column_double          = 4,  // double
    mysql_column_blob            = 5,  // longblob
    mysql_column_datetime        = 6,  // datetime
    mysql_column_auto_increment  = 7,  // bigint(20) auto_increment
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