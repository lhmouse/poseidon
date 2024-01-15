// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_ENUMS_
#define POSEIDON_MYSQL_ENUMS_

#include "../fwd.hpp"
namespace poseidon {

enum MySQL_Data_Type : uint8_t
  {
    mysql_data_varchar   = 0,  // varchar(255)
    mysql_data_boolean   = 1,  // tinyint(1)
    mysql_data_int       = 2,  // int(11)
    mysql_data_bigint    = 3,  // bigint(20)
    mysql_data_autoinc   = 4,  // bigint(20) auto_increment
    mysql_data_double    = 5,  // double
    mysql_data_blob      = 6,  // longblob
    mysql_data_datetime  = 7,  // datetime
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
