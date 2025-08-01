// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_VALUE_
#define POSEIDON_MYSQL_MYSQL_VALUE_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../base/datetime.hpp"
#include "../details/mysql_fwd.hpp"
namespace poseidon {

class MySQL_Value
  {
  private:
    friend class MySQL_Connection;

#define POSEIDON_MYSQL_VALUE_VARIANT_TYPE_  \
    ::rocket::variant<  \
        ::std::nullptr_t, int64_t, double, ::poseidon::cow_string,  \
        ::poseidon::DateTime>

    POSEIDON_MYSQL_VALUE_VARIANT_TYPE_ m_stor;

  public:
    // Value constructors
    constexpr
    MySQL_Value(nullptr_t = nullptr)
      noexcept { }

    MySQL_Value&
    operator=(nullptr_t)
      &
      {
        this->clear();
        return *this;
      }

    MySQL_Value(bool value)
      noexcept
      {
        this->m_stor.emplace<int64_t>(value);
      }

    MySQL_Value&
    operator=(bool value)
      &
      {
        this->m_stor = static_cast<int64_t>(value);
        return *this;
      }

    MySQL_Value(int num)
      noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    MySQL_Value&
    operator=(int num)
      &
      {
        this->m_stor = static_cast<int64_t>(num);
        return *this;
      }

    MySQL_Value(int64_t num)
      noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    MySQL_Value&
    operator=(int64_t num)
      &
      {
        this->m_stor = num;
        return *this;
      }

    MySQL_Value(double num)
      noexcept
      {
        this->m_stor.emplace<double>(num);
      }

    MySQL_Value&
    operator=(double num)
      &
      {
        this->m_stor = num;
        return *this;
      }

    MySQL_Value(const cow_string& str)
      noexcept
      {
        this->m_stor.emplace<cow_string>(str);
      }

    MySQL_Value&
    operator=(const cow_string& str)
      &
      {
        this->m_stor = str;
        return *this;
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    MySQL_Value(const ycharT (*ps)[N])
      noexcept
      {
        this->m_stor.emplace<cow_string>(ps);
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    MySQL_Value&
    operator=(const ycharT (*ps)[N])
      &
      {
        this->open_blob() = ps;
        return *this;
      }

    MySQL_Value(const DateTime& dt)
      noexcept
      {
        this->m_stor.emplace<DateTime>(dt);
      }

    MySQL_Value&
    operator=(const DateTime& dt)
      &
      {
        this->open_datetime() = dt;
        return *this;
      }

    MySQL_Value(system_time tm)
      noexcept
      {
        this->m_stor.emplace<DateTime>(tm);
      }

    MySQL_Value&
    operator=(system_time tm)
      &
      {
        this->open_datetime() = tm;
        return *this;
      }

    MySQL_Value&
    swap(MySQL_Value& other)
      noexcept
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

    MySQL_Value_Type
    type()
      const noexcept
      { return static_cast<MySQL_Value_Type>(this->m_stor.index());  }

    void
    clear()
      noexcept
      { this->m_stor.emplace<nullptr_t>();  }

    // Access raw data.
    bool
    is_null()
      const noexcept
      { return this->m_stor.index() == mysql_value_null;  }

    bool
    is_integer()
      const noexcept
      { return this->m_stor.index() == mysql_value_integer;  }

    const int64_t&
    as_integer()
      const
      { return this->m_stor.as<int64_t>();  }

    int64_t&
    open_integer()
      noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<int64_t>())
          return *ptr;
        else
          return this->m_stor.emplace<int64_t>();
      }

    bool
    is_double()
      const noexcept
      { return this->m_stor.index() == mysql_value_double;  }

    const double&
    as_double()
      const
      { return this->m_stor.as<double>();  }

    double&
    open_double()
      noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<double>())
          return *ptr;
        else
          return this->m_stor.emplace<double>();
      }

    bool
    is_blob()
      const noexcept
      { return this->m_stor.index() == mysql_value_blob;  }

    const cow_string&
    as_blob()
      const
      { return this->m_stor.as<cow_string>();  }

    const char*
    as_blob_data()
      const
      { return this->m_stor.as<cow_string>().data();  }

    size_t
    as_blob_size()
      const
      { return this->m_stor.as<cow_string>().size();  }

    cow_string&
    open_blob()
      noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<cow_string>())
          return *ptr;
        else
          return this->m_stor.emplace<cow_string>();
      }

    bool
    is_datetime()
      const noexcept
      { return this->m_stor.index() == mysql_value_datetime;  }

    const DateTime&
    as_datetime()
      const
      { return this->m_stor.as<DateTime>();  }

    system_time
    as_system_time()
      const
      { return this->m_stor.as<DateTime>().as_system_time();  }

    DateTime&
    open_datetime()
      noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<DateTime>())
          return *ptr;
        else
          return this->m_stor.emplace<DateTime>();
      }

    // Converts this value to its string form. The result will be suitable
    // for immediate use in an SQL statement. Strings are quoted as necessary.
    tinyfmt&
    print_to(tinyfmt& fmt)
      const;

    cow_string
    to_string()
      const;
  };

inline
void
swap(MySQL_Value& lhs, MySQL_Value& rhs)
  noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const MySQL_Value& value)
  { return value.print_to(fmt);  }

}  // namespace poseidon
extern template class POSEIDON_MYSQL_VALUE_VARIANT_TYPE_;
#endif
