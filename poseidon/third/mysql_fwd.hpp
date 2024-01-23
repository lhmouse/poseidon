// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MYSQL_FWD_
#define POSEIDON_THIRD_MYSQL_FWD_

#include "../fwd.hpp"
#include <mysql/mysql.h>
namespace poseidon {

struct MYSQL_deleter
  {
    void
    operator()(::MYSQL* p) const noexcept
      { ::mysql_close(p);  }
  };

struct MYSQL_RES_deleter
  {
    void
    operator()(::MYSQL_RES* p) const noexcept
      { ::mysql_free_result(p);  }
  };

struct MYSQL_STMT_deleter
  {
    void
    operator()(::MYSQL_STMT* p) const noexcept
      { ::mysql_stmt_close(p);  }
  };

using uni_MYSQL = ::rocket::unique_ptr<::MYSQL, MYSQL_deleter>;
using uni_MYSQL_RES = ::rocket::unique_ptr<::MYSQL_RES, MYSQL_RES_deleter>;
using uni_MYSQL_STMT = ::rocket::unique_ptr<::MYSQL_STMT, MYSQL_STMT_deleter>;

}  // namespace poseidon
#endif
