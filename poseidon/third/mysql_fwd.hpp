// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MYSQL_FWD_
#define POSEIDON_THIRD_MYSQL_FWD_

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

struct DateTime_and_MYSQL_TIME
  {
    DateTime dt;
    mutable ::MYSQL_TIME myt;
  };

inline
void
set_mysql_time_to_system_time(::MYSQL_TIME& myt, system_time syst) noexcept
  {
    ::time_t secs = system_clock::to_time_t(syst);
    ::tm tm = { };
    ::gmtime_r(&secs, &tm);

    system_time rounded = system_clock::from_time_t(secs);
    ROCKET_ASSERT(rounded <= syst);
    using ms_type = ::std::chrono::duration<uint32_t, ::std::milli>;
    uint32_t msecs = ::std::chrono::duration_cast<ms_type>(syst - rounded).count();

    myt.year = static_cast<unsigned int>(tm.tm_year) + 1900;
    myt.month = static_cast<unsigned int>(tm.tm_mon) + 1;
    myt.day = static_cast<unsigned int>(tm.tm_mday);
    myt.hour = static_cast<unsigned int>(tm.tm_hour);
    myt.minute = static_cast<unsigned int>(tm.tm_min);
    myt.second = static_cast<unsigned int>(tm.tm_sec);
    myt.second_part = msecs;
    myt.time_type = MYSQL_TIMESTAMP_DATETIME;
  }

}  // namespace poseidon
#endif
