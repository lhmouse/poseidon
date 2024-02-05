// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_VALUE_
#define POSEIDON_MYSQL_MYSQL_VALUE_

#include "../fwd.hpp"
#include "../third/mysql_fwd.hpp"
namespace poseidon {

class MySQL_Value
  {
  private:
    ::rocket::variant<nullptr_t, int64_t, double, cow_string, ::MYSQL_TIME> m_stor;

  public:
    // Value constructors
    constexpr
    MySQL_Value(nullptr_t = nullptr) noexcept
      :
        m_stor()
      { }

    MySQL_Value(bool value) noexcept
      :
        m_stor(static_cast<int64_t>(value))
      { }

    MySQL_Value(int num) noexcept
      :
        m_stor(static_cast<int64_t>(num))
      { }

    MySQL_Value(int64_t num) noexcept
      :
        m_stor(num)
      { }

    MySQL_Value(double num) noexcept
      :
        m_stor(num)
      { }

    MySQL_Value(cow_stringR str) noexcept
      :
        m_stor(str)
      { }

    MySQL_Value(const ::MYSQL_TIME& myt) noexcept
      :
        m_stor(myt)
      { }

    MySQL_Value&
    operator=(nullptr_t) & noexcept
      {
        this->clear();
        return *this;
      }

    MySQL_Value&
    operator=(bool value) & noexcept
      {
        this->set_integer(value);
        return *this;
      }

    MySQL_Value&
    operator=(int num) & noexcept
      {
        this->set_integer(num);
        return *this;
      }

    MySQL_Value&
    operator=(int64_t num) & noexcept
      {
        this->set_integer(num);
        return *this;
      }

    MySQL_Value&
    operator=(double num) & noexcept
      {
        this->set_double(num);
        return *this;
      }

    MySQL_Value&
    operator=(cow_stringR str) & noexcept
      {
        this->set_string(str);
        return *this;
      }

    MySQL_Value&
    operator=(const ::MYSQL_TIME& myt) & noexcept
      {
        this->set_mysql_time(myt);
        return *this;
      }

    MySQL_Value&
    swap(MySQL_Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    MySQL_Value(const MySQL_Value&) = default;
    MySQL_Value(MySQL_Value&&) = default;
    MySQL_Value& operator=(const MySQL_Value&) & = default;
    MySQL_Value& operator=(MySQL_Value&&) & = default;
    ~MySQL_Value();

    // Access raw data.
    bool
    is_null() const noexcept
      { return this->m_stor.ptr<nullptr_t>() != nullptr;  }

    void
    clear() noexcept
      { this->m_stor = nullptr;  }

    bool
    is_integer() const noexcept
      { return this->m_stor.ptr<int64_t>() != nullptr;  }

    const int64_t&
    as_integer() const
      { return this->m_stor.as<int64_t>();  }

    int64_t&
    mut_integer()
      { return this->m_stor.mut<int64_t>();  }

    void
    set_integer(int64_t num) noexcept
      { this->m_stor = num;  }

    bool
    is_double() const noexcept
      { return this->m_stor.ptr<double>() != nullptr;  }

    const double&
    as_double() const
      { return this->m_stor.as<double>();  }

    double&
    mut_double()
      { return this->m_stor.mut<double>();  }

    void
    set_double(double num) noexcept
      { this->m_stor = num;  }

    bool
    is_string() const noexcept
      { return this->m_stor.ptr<cow_string>() != nullptr;  }

    cow_stringR
    as_string() const
      { return this->m_stor.as<cow_string>();  }

    const char*
    str_data() const
      { return this->m_stor.as<cow_string>().c_str();  }

    size_t
    str_length() const
      { return this->m_stor.as<cow_string>().length();  }

    cow_string&
    mut_string()
      { return this->m_stor.mut<cow_string>();  }

    void
    set_string(cow_stringR str) noexcept
      { this->m_stor = str;  }

    void
    set_string(const char* str, size_t len)
      {
        if(auto ps = this->m_stor.mut_ptr<cow_string>())
          ps->assign(str, len);
        else
          this->m_stor.emplace<cow_string>(str, len);
      }

    bool
    is_mysql_time() const noexcept
      { return this->m_stor.ptr<::MYSQL_TIME>() != nullptr;  }

    const ::MYSQL_TIME&
    as_mysql_time() const
      { return this->m_stor.as<::MYSQL_TIME>();  }

    ::MYSQL_TIME&
    mut_mysql_time()
      { return this->m_stor.mut<::MYSQL_TIME>();  }

    void
    set_mysql_time(const ::MYSQL_TIME& myt) noexcept
      { this->m_stor = myt;  }

    void
    set_mysql_datetime(uint32_t y, uint32_t m, uint32_t d, uint32_t H = 0, uint32_t M = 0, uint32_t S = 0, uint32_t ms = 0) noexcept
      {
        ::MYSQL_TIME myt = { };
        myt.year = y;
        myt.month = m;
        myt.day = d;
        myt.hour = H;
        myt.minute = M;
        myt.second = S;
        myt.second_part = ms;
        myt.time_type = MYSQL_TIMESTAMP_DATETIME;
        this->m_stor = myt;
      }

    // Converts this value to its string form. The result will be suitable
    // for immediate use in an SQL statement. Strings are quoted as necessary.
    tinyfmt&
    print(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

inline
void
swap(MySQL_Value& lhs, MySQL_Value& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const MySQL_Value& value)
  { return value.print(fmt);  }

}  // namespace poseidon
#endif
