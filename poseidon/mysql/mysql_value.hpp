// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_MYSQL_VALUE_
#define POSEIDON_MYSQL_MYSQL_VALUE_

#include "../fwd.hpp"
#include "../base/datetime.hpp"
namespace poseidon {

class MySQL_Value
  {
  private:
    ::rocket::variant<nullptr_t, int64_t, double, cow_string, DateTime> m_stor;

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

    MySQL_Value(const DateTime& dt) noexcept
      :
        m_stor(dt)
      { }

    MySQL_Value(system_time tm) noexcept
      :
        m_stor(DateTime(time_point_cast<seconds>(tm)))
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
    operator=(const DateTime& dt) & noexcept
      {
        this->set_datetime(dt);
        return *this;
      }

    MySQL_Value&
    operator=(system_time tm) & noexcept
      {
        this->set_datetime(tm);
        return *this;
      }

    MySQL_Value&
    swap(MySQL_Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(MySQL_Value);

    // Accesses raw data.
    bool
    is_null() const noexcept
      { return this->m_stor.ptr<nullptr_t>() != nullptr;  }

    void
    clear() noexcept
      { this->m_stor = nullptr;  }

    bool
    is_integer() const noexcept
      { return this->m_stor.ptr<int64_t>() != nullptr;  }

    int64_t
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

    double
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
    is_datetime() const noexcept
      { return this->m_stor.ptr<DateTime>() != nullptr;  }

    const DateTime&
    as_datetime() const
      { return this->m_stor.as<DateTime>();  }

    time_t
    as_time_t() const
      { return this->m_stor.as<DateTime>().as_time_t();  }

    DateTime&
    mut_datetime()
      { return this->m_stor.mut<DateTime>();  }

    void
    set_datetime(const DateTime& dt) noexcept
      { this->m_stor = dt;  }

    void
    set_time_t(time_t dt) noexcept
      { this->m_stor.emplace<DateTime>().set_time_t(dt);  }

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
