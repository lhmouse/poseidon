// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_FIELD_NAME_
#define POSEIDON_HTTP_HTTP_FIELD_NAME_

#include "../fwd.hpp"
namespace poseidon {

class HTTP_Field_Name
  {
  public:
    struct hash;

  private:
    cow_string m_str;

  public:
    HTTP_Field_Name()
      noexcept = default;

    template<typename xstringT,
    ROCKET_ENABLE_IF(::std::is_constructible<cow_string, xstringT&&>::value)>
    constexpr
    HTTP_Field_Name(xstringT&& xstr)
      noexcept(::std::is_nothrow_constructible<cow_string, xstringT&&>::value)
      :
        m_str(forward<xstringT>(xstr))
      { }

    template<typename xstringT,
    ROCKET_ENABLE_IF(::std::is_assignable<cow_string&, xstringT&&>::value)>
    HTTP_Field_Name&
    operator=(xstringT&& xstr)
      & noexcept(::std::is_nothrow_assignable<cow_string&, xstringT&&>::value)
      {
        this->m_str = forward<xstringT>(xstr);
        return *this;
      }

    HTTP_Field_Name&
    swap(HTTP_Field_Name& other)
      noexcept
      {
        this->m_str.swap(other.m_str);
        return *this;
      }

  public:
    HTTP_Field_Name(const HTTP_Field_Name&) = default;
    HTTP_Field_Name(HTTP_Field_Name&&) = default;
    HTTP_Field_Name& operator=(const HTTP_Field_Name&) & = default;
    HTTP_Field_Name& operator=(HTTP_Field_Name&&) & = default;
    ~HTTP_Field_Name();

    // accessors
    constexpr
    const cow_string&
    rdstr()
      const noexcept
      { return this->m_str;  }

    operator const cow_string&()
      const noexcept
      { return this->m_str;  }

    constexpr
    const cow_string&
    str()
      const noexcept
      { return this->m_str;  }

    cow_string&
    mut_str()
      noexcept
      { return this->m_str;  }

    constexpr
    bool
    empty()
      const noexcept
      { return this->m_str.empty();  }

    constexpr
    size_t
    size()
      const noexcept
      { return this->m_str.size();  }

    constexpr
    size_t
    length()
      const noexcept
      { return this->m_str.length();  }

    void
    clear()
      noexcept
      { this->m_str.clear();  }

    constexpr
    size_t
    max_size()
      const noexcept
      { return this->m_str.max_size();  }

    constexpr
    ptrdiff_t
    ssize()
      const noexcept
      { return this->m_str.ssize();  }

    constexpr
    size_t
    capacity()
      const noexcept
      { return this->m_str.capacity();  }

    cow_string::const_iterator
    begin()
      const noexcept
      { return this->m_str.begin();  }

    cow_string::const_iterator
    end()
      const noexcept
      { return this->m_str.end();  }

    cow_string::iterator
    mut_begin()
      { return this->m_str.mut_begin();  }

    cow_string::iterator
    mut_end()
      { return this->m_str.mut_end();  }

    cow_string::const_reverse_iterator
    rbegin()
      const noexcept
      { return this->m_str.rbegin();  }

    cow_string::const_reverse_iterator
    rend()
      const noexcept
      { return this->m_str.rend();  }

    cow_string::reverse_iterator
    mut_rbegin()
      { return this->m_str.mut_rbegin();  }

    cow_string::reverse_iterator
    mut_rend()
      { return this->m_str.mut_rend();  }

    constexpr
    const char*
    c_str()
      const noexcept
      { return this->m_str.c_str();  }

    constexpr
    const char*
    data()
      const noexcept
      { return this->m_str.data();  }

    char*
    mut_data()
      { return this->m_str.mut_data();  }

    const char&
    at(size_t pos)
      const
      { return this->m_str.at(pos);  }

    constexpr
    const char&
    operator[](size_t pos)
      const noexcept
      { return this->m_str[pos];  }

    char&
    mut(size_t pos)
      { return this->m_str.mut(pos);  }

    template<typename... xParams>
    void
    assign(xParams&&... xparams)
      { this->m_str.assign(forward<xParams>(xparams)...);  }

    // Compare names in a case-insensitive way.
    constexpr ROCKET_PURE
    bool
    equals(const cow_string& str)
      const noexcept
      {
        if(this->m_str.size() != str.size())
          return false;
        return this->compare(str) == 0;
      }

    constexpr ROCKET_PURE
    bool
    equals(const HTTP_Field_Name& other)
      const noexcept
      {
        if(this->m_str.size() != other.m_str.size())
          return false;
        return this->compare(other) == 0;
      }

    ROCKET_PURE
    int
    compare(const cow_string& cmps)
      const noexcept;

    ROCKET_PURE
    int
    compare(const HTTP_Field_Name& other)
      const noexcept;

    // Gets the case-insensitive hash value of this name.
    ROCKET_PURE
    size_t
    rdhash()
      const noexcept;

    // Validates this name and converts all ASCII letters to lowercase. If this
    // string is not a valid HTTP token, an exception is thrown, and the string
    // may have been partially modified.
    void
    canonicalize();
  };

struct HTTP_Field_Name::hash
  {
    size_t
    operator()(const HTTP_Field_Name& name)
      const noexcept
      { return name.rdhash();  }
  };

inline
void
swap(HTTP_Field_Name& lhs, HTTP_Field_Name& rhs)
  noexcept
  { lhs.swap(rhs);  }

inline
bool
operator==(const HTTP_Field_Name& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return lhs.equals(rhs);  }

inline
bool
operator==(const HTTP_Field_Name& lhs, const cow_string& rhs)
  noexcept
  { return lhs.equals(rhs);  }

inline
bool
operator==(const HTTP_Field_Name& lhs, const char* rhs)
  noexcept
  { return lhs.equals(rhs);  }

inline
bool
operator==(const cow_string& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.equals(lhs);  }

inline
bool
operator==(const char* lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.equals(lhs);  }

inline
bool
operator!=(const HTTP_Field_Name& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return !lhs.equals(rhs);  }

inline
bool
operator!=(const HTTP_Field_Name& lhs, const cow_string& rhs)
  noexcept
  { return !lhs.equals(rhs);  }

inline
bool
operator!=(const HTTP_Field_Name& lhs, const char* rhs)
  noexcept
  { return !lhs.equals(rhs);  }

inline
bool
operator!=(const cow_string& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return !rhs.equals(lhs);  }

inline
bool
operator!=(const char* lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return !rhs.equals(lhs);  }

inline
bool
operator<(const HTTP_Field_Name& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return lhs.compare(rhs) < 0;  }

inline
bool
operator<(const HTTP_Field_Name& lhs, const cow_string& rhs)
  noexcept
  { return lhs.compare(rhs) < 0;  }

inline
bool
operator<(const HTTP_Field_Name& lhs, const char* rhs)
  noexcept
  { return lhs.compare(rhs) < 0;  }

inline
bool
operator<(const cow_string& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) > 0;  }

inline
bool
operator<(const char* lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) > 0;  }

inline
bool
operator<=(const HTTP_Field_Name& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return lhs.compare(rhs) <= 0;  }

inline
bool
operator<=(const HTTP_Field_Name& lhs, const cow_string& rhs)
  noexcept
  { return lhs.compare(rhs) <= 0;  }

inline
bool
operator<=(const HTTP_Field_Name& lhs, const char* rhs)
  noexcept
  { return lhs.compare(rhs) <= 0;  }

inline
bool
operator<=(const cow_string& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) >= 0;  }

inline
bool
operator<=(const char* lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) >= 0;  }

inline
bool
operator>(const HTTP_Field_Name& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return lhs.compare(rhs) > 0;  }

inline
bool
operator>(const HTTP_Field_Name& lhs, const cow_string& rhs)
  noexcept
  { return lhs.compare(rhs) > 0;  }

inline
bool
operator>(const HTTP_Field_Name& lhs, const char* rhs)
  noexcept
  { return lhs.compare(rhs) > 0;  }

inline
bool
operator>(const cow_string& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) < 0;  }

inline
bool
operator>(const char* lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) < 0;  }

inline
bool
operator>=(const HTTP_Field_Name& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return lhs.compare(rhs) >= 0;  }

inline
bool
operator>=(const HTTP_Field_Name& lhs, const cow_string& rhs)
  noexcept
  { return lhs.compare(rhs) >= 0;  }

inline
bool
operator>=(const HTTP_Field_Name& lhs, const char* rhs)
  noexcept
  { return lhs.compare(rhs) >= 0;  }

inline
bool
operator>=(const cow_string& lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) <= 0;  }

inline
bool
operator>=(const char* lhs, const HTTP_Field_Name& rhs)
  noexcept
  { return rhs.compare(lhs) <= 0;  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const HTTP_Field_Name& name)
  { return fmt << name.rdstr();  }

}  // namespace poseidon
#endif
