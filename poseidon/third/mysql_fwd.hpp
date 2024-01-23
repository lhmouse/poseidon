// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MYSQL_FWD_
#define POSEIDON_THIRD_MYSQL_FWD_

#include "../fwd.hpp"
#include <mysql/mysql.h>
namespace poseidon {

// class unique_MYSQL
using ::MYSQL;
POSEIDON_DEFINE_unique(MYSQL, ::mysql_close);

// class unique_MYSQL_STMT
using ::MYSQL_STMT;
POSEIDON_DEFINE_unique(MYSQL_STMT, ::mysql_stmt_close);

// class unique_MYSQL_RES
using ::MYSQL_RES;
POSEIDON_DEFINE_unique(MYSQL_RES, ::mysql_free_result);

}  // namespace poseidon
#endif
