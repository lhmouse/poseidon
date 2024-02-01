// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MYSQL_FWD_
#define POSEIDON_THIRD_MYSQL_FWD_

#include "../fwd.hpp"
#include <mysql/mysql.h>
namespace poseidon {

class mysql_Client
  {
  private:
    mutable ::MYSQL m_mysql[1];

  public:
    mysql_Client()
      {
        if(::mysql_init(this->m_mysql) == nullptr)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                "mysql_Client: insufficient memory");

        // Set default options.
        ::mysql_options(this->m_mysql, MYSQL_OPT_COMPRESS, "1");
        ::mysql_options(this->m_mysql, MYSQL_SET_CHARSET_NAME , "utf8mb4");
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(mysql_Client)
      {
        ::mysql_close(this->m_mysql);
      }

  public:
    operator
    ::MYSQL*() const noexcept
      { return this->m_mysql;  }
  };

struct MYSQL_STMT_deleter
  {
    void
    operator()(::MYSQL_STMT* p) const noexcept
      { ::mysql_stmt_close(p);  }
  };

struct MYSQL_RES_deleter
  {
    void
    operator()(::MYSQL_RES* p) const noexcept
      { ::mysql_free_result(p);  }
  };

using uni_MYSQL_STMT = ::rocket::unique_ptr<::MYSQL_STMT, MYSQL_STMT_deleter>;
using uni_MYSQL_RES = ::rocket::unique_ptr<::MYSQL_RES, MYSQL_RES_deleter>;

}  // namespace poseidon
#endif
