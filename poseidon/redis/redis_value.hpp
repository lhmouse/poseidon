// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_REDIS_REDIS_VALUE_
#define POSEIDON_REDIS_REDIS_VALUE_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

class Redis_Value
  {
  private:
    friend class Redis_Connection;

    ::rocket::variant<nullptr_t,
         int64_t, cow_string, Redis_Array> m_stor;

  public:
    // Value constructors
    constexpr Redis_Value(nullptr_t = nullptr) noexcept { }

    Redis_Value(int num) noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    Redis_Value&
    operator=(int num) noexcept
      {
        this->m_stor = static_cast<int64_t>(num);
        return *this;
      }

    Redis_Value(int64_t num) noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    Redis_Value&
    operator=(int64_t num) noexcept
      {
        this->m_stor = num;
        return *this;
      }

    Redis_Value(const cow_string& str) noexcept
      {
        this->m_stor.emplace<cow_string>(str);
      }

    Redis_Value&
    operator=(const cow_string& str) noexcept
      {
        this->m_stor = str;
        return *this;
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    Redis_Value(const ycharT (*ps)[N]) noexcept
      {
        this->m_stor.emplace<cow_string>(ps);
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    Redis_Value&
    operator=(const ycharT (*ps)[N]) noexcept
      {
        this->mut_string() = ps;
        return *this;
      }

    Redis_Value(const Redis_Array& arr) noexcept
      {
        this->m_stor.emplace<Redis_Array>(arr);
      }

    Redis_Value&
    operator=(const Redis_Array& arr) noexcept
      {
        this->m_stor = arr;
        return *this;
      }

    Redis_Value&
    swap(Redis_Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    Redis_Value(const Redis_Value&) = default;
    Redis_Value(Redis_Value&&) = default;
    Redis_Value& operator=(const Redis_Value&) & = default;
    Redis_Value& operator=(Redis_Value&&) & = default;
    ~Redis_Value();

    Redis_Value_Type
    type() const noexcept
      { return static_cast<Redis_Value_Type>(this->m_stor.index());  }

    void
    clear() noexcept
      { this->m_stor.emplace<nullptr_t>();  }

    // Access raw data.
    bool
    is_null() const noexcept
      { return this->m_stor.index() == redis_value_null;  }

    bool
    is_integer() const noexcept
      { return this->m_stor.index() == redis_value_integer;  }

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
    is_string() const noexcept
      { return this->m_stor.index() == redis_value_string;  }

    const cow_string&
    as_string() const
      { return this->m_stor.as<cow_string>();  }

    const char*
    as_string_c_str() const
      { return this->m_stor.as<cow_string>().c_str();  }

    size_t
    as_string_length() const
      { return this->m_stor.as<cow_string>().length();  }

    cow_string&
    mut_string() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<cow_string>())
          return *ptr;
        else
          return this->m_stor.emplace<cow_string>();
      }

    bool
    is_array() const noexcept
      { return this->m_stor.index() == redis_value_array;  }

    const Redis_Array&
    as_array() const
      { return this->m_stor.as<Redis_Array>();  }

    Redis_Array&
    mut_array() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<Redis_Array>())
          return *ptr;
        else
          return this->m_stor.emplace<Redis_Array>();
      }

    // Converts this value to its string form. Strings are enclosed in double
    // quotation marks.
    tinyfmt&
    print_to(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

inline
void
swap(Redis_Value& lhs, Redis_Value& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Redis_Value& value)
  { return value.print_to(fmt);  }

}  // namespace poseidon
#endif
