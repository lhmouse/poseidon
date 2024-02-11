// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

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
    ::rocket::variant<nullptr_t, bool, int64_t, double,
         cow_string, Mongo_Binary, Mongo_Array, Mongo_Document,
         ::bson_oid_t, DateTime> m_stor;

  public:
    // Value constructors
    constexpr Mongo_Value(nullptr_t = nullptr) noexcept { }

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

    Mongo_Value(cow_stringR utf8) noexcept
      {
        this->m_stor.emplace<cow_string>(utf8);
      }

    Mongo_Value&
    operator=(cow_stringR utf8) noexcept
      {
        this->m_stor = utf8;
        return *this;
      }

    template<size_t N>
    Mongo_Value(const char (*ps)[N]) noexcept
      {
        this->m_stor.emplace<cow_string>(ps);
      }

    template<size_t N>
    Mongo_Value&
    operator=(const char (*ps)[N]) noexcept
      {
        this->mut_utf8() = ps;
        return *this;
      }

    Mongo_Value(const Mongo_Binary& bin) noexcept
      {
        this->m_stor.emplace<Mongo_Binary>(bin);
      }

    Mongo_Value&
    operator=(const Mongo_Binary& bin) noexcept
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

    // Access raw data.
    bool
    is_null() const noexcept
      { return this->m_stor.ptr<nullptr_t>() != nullptr;  }

    void
    clear() noexcept
      { this->m_stor = nullptr;  }

    bool
    is_boolean() const noexcept
      { return this->m_stor.ptr<bool>() != nullptr;  }

    const bool&
    as_boolean() const
      { return this->m_stor.as<bool>();  }

    bool&
    mut_boolean() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<bool>())
          return *ptr;
        else
          return this->m_stor.emplace<bool>();
      }

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
    is_utf8() const noexcept
      { return this->m_stor.ptr<cow_string>() != nullptr;  }

    cow_stringR
    as_utf8() const
      { return this->m_stor.as<cow_string>();  }

    const char*
    utf8_data() const
      { return this->m_stor.as<cow_string>().data();  }

    size_t
    utf8_size() const
      { return this->m_stor.as<cow_string>().size();  }

    cow_string&
    mut_utf8() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<cow_string>())
          return *ptr;
        else
          return this->m_stor.emplace<cow_string>();
      }

    bool
    is_binary() const noexcept
      { return this->m_stor.ptr<Mongo_Binary>() != nullptr;  }

    const Mongo_Binary&
    as_binary() const
      { return this->m_stor.as<Mongo_Binary>();  }

    const uint8_t*
    binary_data() const
      { return this->m_stor.as<Mongo_Binary>().data();  }

    size_t
    binary_size() const
      { return this->m_stor.as<Mongo_Binary>().size();  }

    Mongo_Binary&
    mut_binary() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<Mongo_Binary>())
          return *ptr;
        else
          return this->m_stor.emplace<Mongo_Binary>();
      }

    bool
    is_array() const noexcept
      { return this->m_stor.ptr<Mongo_Array>() != nullptr;  }

    const Mongo_Array&
    as_array() const
      { return this->m_stor.as<Mongo_Array>();  }

    Mongo_Array&
    mut_array() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<Mongo_Array>())
          return *ptr;
        else
          return this->m_stor.emplace<Mongo_Array>();
      }

    bool
    is_document() const noexcept
      { return this->m_stor.ptr<Mongo_Document>() != nullptr;  }

    const Mongo_Document&
    as_document() const
      { return this->m_stor.as<Mongo_Document>();  }

    Mongo_Document&
    mut_document() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<Mongo_Document>())
          return *ptr;
        else
          return this->m_stor.emplace<Mongo_Document>();
      }

    bool
    is_oid() const noexcept
      { return this->m_stor.ptr<::bson_oid_t>() != nullptr;  }

    const ::bson_oid_t&
    as_oid() const
      { return this->m_stor.as<::bson_oid_t>();  }

    ::bson_oid_t&
    mut_oid() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<::bson_oid_t>())
          return *ptr;
        else
          return this->m_stor.emplace<::bson_oid_t>();
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
    mut_datetime() noexcept
      {
        if(auto ptr = this->m_stor.mut_ptr<DateTime>())
          return *ptr;
        else
          return this->m_stor.emplace<DateTime>();
      }

    // Converts this value to its string form, as an extended JSON string.
    // https://github.com/mongodb/specifications/blob/master/source/extended-json.rst#conversion-table
    tinyfmt&
    print(tinyfmt& fmt) const;

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
  { return value.print(fmt);  }

}  // namespace poseidon
#endif
