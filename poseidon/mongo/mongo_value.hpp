// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MONGO_MONGO_VALUE_
#define POSEIDON_MONGO_MONGO_VALUE_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../base/datetime.hpp"
#include "../third/mongo_fwd.hpp"
namespace poseidon {

class Mongo_Value
  {
  private:
    friend class Mongo_Connection;

    ::rocket::variant<nullptr_t, bool, int64_t, double,
         cow_string, cow_bstring, Mongo_Array, Mongo_Document,
         ::bson_oid_t, DateTime> m_stor;

  public:
    // Value constructors
    constexpr
    Mongo_Value(nullptr_t = nullptr) noexcept { }

    Mongo_Value(bool value) noexcept
      {
        this->m_stor.emplace<bool>(value);
      }

    Mongo_Value&
    operator=(bool value) noexcept
      {
        this->m_stor = value;
        return *this;
      }

    Mongo_Value(int num) noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    Mongo_Value&
    operator=(int num) noexcept
      {
        this->m_stor = static_cast<int64_t>(num);
        return *this;
      }

    Mongo_Value(int64_t num) noexcept
      {
        this->m_stor.emplace<int64_t>(num);
      }

    Mongo_Value&
    operator=(int64_t num) noexcept
      {
        this->m_stor = num;
        return *this;
      }

    Mongo_Value(double num) noexcept
      {
        this->m_stor.emplace<double>(num);
      }

    Mongo_Value&
    operator=(double num) noexcept
      {
        this->m_stor = num;
        return *this;
      }

    Mongo_Value(const cow_string& utf8) noexcept
      {
        this->m_stor.emplace<cow_string>(utf8);
      }

    Mongo_Value&
    operator=(const cow_string& utf8) noexcept
      {
        this->m_stor = utf8;
        return *this;
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    Mongo_Value(const ycharT (*ps)[N]) noexcept
      {
        this->m_stor.emplace<cow_string>(ps);
      }

    template<typename ycharT, size_t N,
    ROCKET_ENABLE_IF(::std::is_same<ycharT, char>::value)>
    Mongo_Value&
    operator=(const ycharT (*ps)[N]) noexcept
      {
        this->open_utf8() = ps;
        return *this;
      }

    Mongo_Value(const cow_bstring& bin) noexcept
      {
        this->m_stor.emplace<cow_bstring>(bin);
      }

    Mongo_Value&
    operator=(const cow_bstring& bin) noexcept
      {
        this->m_stor = bin;
        return *this;
      }

    Mongo_Value(const Mongo_Array& arr) noexcept
      {
        this->m_stor.emplace<Mongo_Array>(arr);
      }

    Mongo_Value&
    operator=(const Mongo_Array& arr) noexcept
      {
        this->m_stor = arr;
        return *this;
      }

    Mongo_Value(const Mongo_Document& doc) noexcept
      {
        this->m_stor.emplace<Mongo_Document>(doc);
      }

    Mongo_Value&
    operator=(const Mongo_Document& doc) noexcept
      {
        this->m_stor = doc;
        return *this;
      }

    Mongo_Value(const ::bson_oid_t& oid) noexcept
      {
        this->m_stor.emplace<::bson_oid_t>(oid);
      }

    Mongo_Value&
    operator=(const ::bson_oid_t& oid) noexcept
      {
        this->m_stor = oid;
        return *this;
      }

    Mongo_Value(const DateTime& dt) noexcept
      {
        this->m_stor.emplace<DateTime>(dt);
      }

    Mongo_Value&
    operator=(const DateTime& dt) & noexcept
      {
        this->m_stor = dt;
        return *this;
      }

    Mongo_Value(system_time tm) noexcept
      {
        this->m_stor.emplace<DateTime>(tm);
      }

    Mongo_Value&
    operator=(system_time tm) & noexcept
      {
        this->m_stor.emplace<DateTime>(tm);
        return *this;
      }

    Mongo_Value&
    swap(Mongo_Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

  public:
    Mongo_Value(const Mongo_Value&) = default;
    Mongo_Value(Mongo_Value&&) = default;
    Mongo_Value& operator=(const Mongo_Value&) & = default;
    Mongo_Value& operator=(Mongo_Value&&) & = default;
    ~Mongo_Value();

    Mongo_Value_Type
    type() const noexcept
      { return static_cast<Mongo_Value_Type>(this->m_stor.index());  }

    void
    clear() noexcept
      { this->m_stor.emplace<nullptr_t>();  }

    // Access raw data.
    bool
    is_null() const noexcept
      { return this->m_stor.index() == mongo_value_null;  }

    bool
    is_boolean() const noexcept
      { return this->m_stor.index() == mongo_value_boolean;  }

    const bool&
    as_boolean() const
      { return this->m_stor.as<bool>();  }

    bool&
    open_boolean() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<bool>())
          return *ptr;
        else
          return this->m_stor.emplace<bool>();
      }

    bool
    is_integer() const noexcept
      { return this->m_stor.index() == mongo_value_integer;  }

    const int64_t&
    as_integer() const
      { return this->m_stor.as<int64_t>();  }

    int64_t&
    open_integer() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<int64_t>())
          return *ptr;
        else
          return this->m_stor.emplace<int64_t>();
      }

    bool
    is_double() const noexcept
      { return this->m_stor.index() == mongo_value_double;  }

    const double&
    as_double() const
      { return this->m_stor.as<double>();  }

    double&
    open_double() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<double>())
          return *ptr;
        else
          return this->m_stor.emplace<double>();
      }

    bool
    is_utf8() const noexcept
      { return this->m_stor.index() == mongo_value_utf8;  }

    const cow_string&
    as_utf8() const
      { return this->m_stor.as<cow_string>();  }

    const char*
    as_utf8_c_str() const
      { return this->m_stor.as<cow_string>().c_str();  }

    size_t
    as_utf8_length() const
      { return this->m_stor.as<cow_string>().length();  }

    cow_string&
    open_utf8() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<cow_string>())
          return *ptr;
        else
          return this->m_stor.emplace<cow_string>();
      }

    bool
    is_binary() const noexcept
      { return this->m_stor.index() == mongo_value_binary;  }

    const cow_bstring&
    as_binary() const
      { return this->m_stor.as<cow_bstring>();  }

    const uint8_t*
    as_binary_data() const
      { return this->m_stor.as<cow_bstring>().data();  }

    size_t
    as_binary_size() const
      { return this->m_stor.as<cow_bstring>().size();  }

    cow_bstring&
    open_binary() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<cow_bstring>())
          return *ptr;
        else
          return this->m_stor.emplace<cow_bstring>();
      }

    bool
    is_array() const noexcept
      { return this->m_stor.index() == mongo_value_array;  }

    const Mongo_Array&
    as_array() const
      { return this->m_stor.as<Mongo_Array>();  }

    Mongo_Array&
    open_array() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<Mongo_Array>())
          return *ptr;
        else
          return this->m_stor.emplace<Mongo_Array>();
      }

    bool
    is_document() const noexcept
      { return this->m_stor.index() == mongo_value_document;  }

    const Mongo_Document&
    as_document() const
      { return this->m_stor.as<Mongo_Document>();  }

    Mongo_Document&
    open_document() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<Mongo_Document>())
          return *ptr;
        else
          return this->m_stor.emplace<Mongo_Document>();
      }

    bool
    is_oid() const noexcept
      { return this->m_stor.index() == mongo_value_oid;  }

    const ::bson_oid_t&
    as_oid() const
      { return this->m_stor.as<::bson_oid_t>();  }

    ::bson_oid_t&
    open_oid() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<::bson_oid_t>())
          return *ptr;
        else
          return this->m_stor.emplace<::bson_oid_t>();
      }

    bool
    is_datetime() const noexcept
      { return this->m_stor.index() == mongo_value_datetime;  }

    const DateTime&
    as_datetime() const
      { return this->m_stor.as<DateTime>();  }

    system_time
    as_system_time() const
      { return this->m_stor.as<DateTime>().as_system_time();  }

    DateTime&
    open_datetime() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<DateTime>())
          return *ptr;
        else
          return this->m_stor.emplace<DateTime>();
      }

    // Converts this value to its string form, as an extended JSON string. This
    // notation does not distinguish 64-bit integers from floating-point values.
    tinyfmt&
    print_to(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

inline
void
swap(Mongo_Value& lhs, Mongo_Value& rhs) noexcept
  { lhs.swap(rhs);  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const Mongo_Value& value)
  { return value.print_to(fmt);  }

}  // namespace poseidon
#endif
