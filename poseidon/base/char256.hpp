// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_CHAR256_
#define POSEIDON_BASE_CHAR256_

#include "../fwd.hpp"
namespace poseidon {

class char256
  {
  private:
    union {
      char m_data[8];
      ::std::aligned_storage<256>::type m_stor;
    };

  public:
    // Constructs a null-terminated string of zero characters.
    // This constructor is not explicit as it doesn't allocate memory.
    constexpr
    char256() noexcept
      : m_data()
      { }

    // Constructs a null-terminated string.
    // This constructor is not explicit as it doesn't allocate memory.
    constexpr
    char256(const char* str_opt)
      : m_data()
      {
        const char* str = str_opt ? str_opt : "";
        size_t len = ::rocket::xstrlen(str);
        if(len >= 256)
          ::rocket::sprintf_and_throw<::std::length_error>(
              "char256: string `%s` (length `%lld`) too long",
              str, (long long) len);

        // XXX: This should be `xmempcpy()` but clang doesn't like it?
        ::rocket::details_xstring::maybe_constexpr::ymempcpy(this->m_data, str, len + 1);
      }

    char256&
    swap(char256& other) noexcept
      {
        ::std::swap(this->m_stor, other.m_stor);
        return *this;
      }

  public:
    // Returns a pointer to internal storage so a buffer can be passed as
    // an argument for `char*`.
    constexpr operator
    const char*() const noexcept
      { return this->m_data;  }

    operator
    char*() noexcept
      { return this->m_data;  }

    constexpr
    const char*
    c_str() const noexcept
      { return this->m_data;  }
  };

inline
void
swap(char256& lhs, char256& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const char256& cbuf)
  {
    return fmt << cbuf.c_str();
  }

constexpr
bool
operator==(const char256& lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs.c_str()) == 0;
  }

constexpr
bool
operator==(const char* lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs, rhs.c_str()) == 0;
  }

constexpr
bool
operator==(const char256& lhs, const char* rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs) == 0;
  }

constexpr
bool
operator!=(const char256& lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs.c_str()) != 0;
  }

constexpr
bool
operator!=(const char* lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs, rhs.c_str()) != 0;
  }

constexpr
bool
operator!=(const char256& lhs, const char* rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs) != 0;
  }

constexpr
bool
operator<(const char256& lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs.c_str()) < 0;
  }

constexpr
bool
operator<(const char* lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs, rhs.c_str()) < 0;
  }

constexpr
bool
operator<(const char256& lhs, const char* rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs) < 0;
  }

constexpr
bool
operator>(const char256& lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs.c_str()) > 0;
  }

constexpr
bool
operator>(const char* lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs, rhs.c_str()) > 0;
  }

constexpr
bool
operator>(const char256& lhs, const char* rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs) > 0;
  }

constexpr
bool
operator<=(const char256& lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs.c_str()) <= 0;
  }

constexpr
bool
operator<=(const char* lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs, rhs.c_str()) <= 0;
  }

constexpr
bool
operator<=(const char256& lhs, const char* rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs) <= 0;
  }

constexpr
bool
operator>=(const char256& lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs.c_str()) >= 0;
  }

constexpr
bool
operator>=(const char* lhs, const char256& rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs, rhs.c_str()) >= 0;
  }

constexpr
bool
operator>=(const char256& lhs, const char* rhs) noexcept
  {
    return ::rocket::xstrcmp(lhs.c_str(), rhs) >= 0;
  }

}  // namespace poseidon
#endif
