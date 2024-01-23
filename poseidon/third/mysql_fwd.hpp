// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MYSQL_FWD_
#define POSEIDON_THIRD_MYSQL_FWD_

#include "../fwd.hpp"
#include <mysql/mysql.h>
namespace poseidon {

class MySQL_Client
  {
  private:
    mutable ::MYSQL m_mysql[1];

  public:
    explicit
    MySQL_Client()
      {
        if(::mysql_init(this->m_mysql) == nullptr)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                "MySQL_Client: insufficient memory");

        // Set default options.
        ::mysql_options(this->m_mysql, MYSQL_OPT_COMPRESS, "1");
        ::mysql_options(this->m_mysql, MYSQL_SET_CHARSET_NAME , "utf8mb4");
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(MySQL_Client)
      {
        ::mysql_close(this->m_mysql);
      }

    [[noreturn]]
    void
    throw_exception() const
      {
        ::rocket::sprintf_and_throw<::std::runtime_error>(
              "MySQL_Client: ERROR %u (%s): %s",
              ::mysql_errno(this->m_mysql), ::mysql_sqlstate(this->m_mysql),
              ::mysql_error(this->m_mysql));
      }
  };

}  // namespace poseidon
#endif
