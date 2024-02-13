// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MYSQL_FWD_
#define POSEIDON_THIRD_MYSQL_FWD_

#include "../fwd.hpp"
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
                "mysql_Client: insufficient memory");

        // Set default options.
        ::mysql_options(this->m_mysql, MYSQL_OPT_COMPRESS, "1");
        ::mysql_options(this->m_mysql, MYSQL_SET_CHARSET_NAME , "utf8mb4");
      }

    ~scoped_MYSQL()
      {
        ::mysql_close(this->m_mysql);
      }

    scoped_MYSQL(const scoped_MYSQL&) = delete;
    scoped_MYSQL& operator=(const scoped_MYSQL&) & = delete;

    operator ::MYSQL*() const noexcept
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

using uniptr_MYSQL_STMT = ::rocket::unique_ptr<::MYSQL_STMT, MYSQL_STMT_deleter>;
using uniptr_MYSQL_RES = ::rocket::unique_ptr<::MYSQL_RES, MYSQL_RES_deleter>;

inline
void
set_tm_from_mysql_time(struct ::tm& tm, const ::MYSQL_TIME& myt) noexcept
  {
    ::memset(&tm, 0, sizeof(struct ::tm));
    tm.tm_year = static_cast<int>(myt.year - 1900);
    tm.tm_mon = static_cast<int>(myt.month - 1);
    tm.tm_mday = static_cast<int>(myt.day);
    tm.tm_hour = static_cast<int>(myt.hour);
    tm.tm_min = static_cast<int>(myt.minute);
    tm.tm_sec = static_cast<int>(myt.second);
    tm.tm_isdst = -1;
  }

inline
void
set_mysql_time_from_tm(::MYSQL_TIME& myt, const struct ::tm& tm) noexcept
  {
    ::memset(&myt, 0, sizeof(::MYSQL_TIME));
    myt.year = static_cast<unsigned>(tm.tm_year) + 1900;
    myt.month = static_cast<unsigned>(tm.tm_mon) + 1;
    myt.day = static_cast<unsigned>(tm.tm_mday);
    myt.hour = static_cast<unsigned>(tm.tm_hour);
    myt.minute = static_cast<unsigned>(tm.tm_min);
    myt.second = static_cast<unsigned>(tm.tm_sec);
    myt.time_type = MYSQL_TIMESTAMP_DATETIME;
  }

}  // namespace poseidon
#endif
