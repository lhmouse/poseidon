// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_VALUE_
#define POSEIDON_MYSQL_MYSQL_VALUE_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../base/datetime.hpp"
#include "../third/mysql_fwd.hpp"
namespace poseidon {

class MySQL_Value
  {
  private:
    friend class MySQL_Connection;

    ::rocket::variant<nullptr_t,
         int64_t, double, cow_string, DateTime_and_MYSQL_TIME> m_stor;

  public:
    // Value constructors
    constexpr MySQL_Value(nullptr_t = nullptr) noexcept { }

    MySQL_Value&
    operator=(nullptr_t) & noexcept
      {
        this->clear();
        return *this;
      }

    MySQL_Value(bool value) noexcept
      {
        this->m_stor.emplace<int64_t>(value);
      }

    MySQL_Value&
    operator=(bool value) & noexcept
      {
        this->m_stor = static_cast<int64_t>(value);
        return *this;
      }

    MySQL_Value(int num) noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    MySQL_Value&
    operator=(int num) & noexcept
      {
        this->m_stor = static_cast<int64_t>(num);
        return *this;
      }

    MySQL_Value(int64_t num) noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    MySQL_Value&
    operator=(int64_t num) & noexcept
      {
        this->m_stor = num;
        return *this;
      }

    MySQL_Value(double num) noexcept
      {
        this->m_stor.emplace<double>(num);
      }

    MySQL_Value&
    operator=(double num) & noexcept
      {
        this->m_stor = num;
        return *this;
      }

    MySQL_Value(cow_stringR str) noexcept
      {
        this->m_stor.emplace<cow_string>(str);
      }

    MySQL_Value&
    operator=(cow_stringR str) & noexcept
      {
        this->m_stor = str;
        return *this;
      }

    template<size_t N>
    MySQL_Value(const char (*ps)[N]) noexcept
      {
        this->m_stor.emplace<cow_string>(ps);
      }

    template<size_t N>
    MySQL_Value&
    operator=(const char (*ps)[N]) noexcept
      {
        this->mut_blob() = ps;
        return *this;
      }

    MySQL_Value(const DateTime& dt) noexcept
      {
        this->m_stor.emplace<DateTime_and_MYSQL_TIME>().dt = dt;
      }

    MySQL_Value&
    operator=(const DateTime& dt) & noexcept
      {
        this->m_stor.emplace<DateTime_and_MYSQL_TIME>().dt = dt;
        return *this;
      }

    MySQL_Value(system_time tm) noexcept
      {
        this->m_stor.emplace<DateTime_and_MYSQL_TIME>().dt = tm;
      }

    MySQL_Value&
    operator=(system_time tm) & noexcept
      {
        this->m_stor.emplace<DateTime_and_MYSQL_TIME>().dt = tm;
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
    mut_integer() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<int64_t>())
          return *ptr;
        else
          return this->m_stor.emplace<int64_t>();
      }

    bool
    is_double() const noexcept
      { return this->m_stor.ptr<double>() != nullptr;  }

    const double&
    as_double() const
      { return this->m_stor.as<double>();  }

    double&
    mut_double() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<double>())
          return *ptr;
        else
          return this->m_stor.emplace<double>();
      }

    bool
    is_blob() const noexcept
      { return this->m_stor.ptr<cow_string>() != nullptr;  }

    cow_stringR
    as_blob() const
      { return this->m_stor.as<cow_string>();  }

    const char*
    blob_data() const
      { return this->m_stor.as<cow_string>().data();  }

    size_t
    blob_size() const
      { return this->m_stor.as<cow_string>().size();  }

    cow_string&
    mut_blob() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<cow_string>())
          return *ptr;
        else
          return this->m_stor.emplace<cow_string>();
      }

    bool
    is_datetime() const noexcept
      { return this->m_stor.ptr<DateTime_and_MYSQL_TIME>() != nullptr;  }

    const DateTime&
    as_datetime() const
      { return this->m_stor.as<DateTime_and_MYSQL_TIME>().dt;  }

    system_time
    as_system_time() const
      { return this->m_stor.as<DateTime_and_MYSQL_TIME>().dt.as_system_time();  }

    DateTime&
    mut_datetime() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<DateTime_and_MYSQL_TIME>())
          return ptr->dt;
        else
          return this->m_stor.emplace<DateTime_and_MYSQL_TIME>().dt;
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
