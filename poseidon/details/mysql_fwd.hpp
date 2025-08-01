// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_DETAILS_MYSQL_FWD_
#define POSEIDON_DETAILS_MYSQL_FWD_

#include "../fwd.hpp"
#include "../base/datetime.hpp"
#include <mysql/mysql.h>
namespace poseidon {

class scoped_MYSQL
  {
  private:
    mutable ::MYSQL m_mysql[1];

  public:
    scoped_MYSQL()
      {
        if(::mysql_init(this->m_mysql) == nullptr)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                "scoped_MYSQL: insufficient memory");

        ::mysql_options(this->m_mysql, MYSQL_OPT_COMPRESS, "1");
        ::mysql_options(this->m_mysql, MYSQL_SET_CHARSET_NAME , "utf8mb4");
      }

    ~scoped_MYSQL()
      {
        ::mysql_close(this->m_mysql);
      }

    scoped_MYSQL(const scoped_MYSQL&) = delete;
    scoped_MYSQL& operator=(const scoped_MYSQL&) & = delete;

    operator ::MYSQL*()
      const noexcept
      { return this->m_mysql;  }
  };

struct MYSQL_STMT_deleter
  {
    void
    operator()(::MYSQL_STMT* p)
      const noexcept
      { ::mysql_stmt_close(p);  }
  };

struct MYSQL_RES_deleter
  {
    void
    operator()(::MYSQL_RES* p)
      const noexcept
      { ::mysql_free_result(p);  }
  };

using uniptr_MYSQL_STMT = ::rocket::unique_ptr<::MYSQL_STMT, MYSQL_STMT_deleter>;
using uniptr_MYSQL_RES = ::rocket::unique_ptr<::MYSQL_RES, MYSQL_RES_deleter>;

}  // namespace poseidon
#endif
