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
      char m_first;
      char m_data[256];
      ::std::aligned_storage<256>::type m_stor;
    };

  public:
    // Constructs a null-terminated string of zero characters.
    // This constructor is not explicit as it doesn't allocate memory.
    constexpr
    char256() noexcept
      : m_first()
      { }

    // Constructs a null-terminated string.
    // This constructor is not explicit as it doesn't allocate memory.
    char256(const char* str_opt)
      {
        const char* str = str_opt ? str_opt : "";
        size_t len = ::rocket::xstrlen(str);
        if(len >= 256)
          ::rocket::sprintf_and_throw<::std::length_error>(
              "char256: string `%s` (length `%lld`) too long",
              str, (long long) len);

        ::memcpy(this->m_data, str, len + 1);
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

inline
bool
operator==(const char256& lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) == 0;
  }

inline
bool
operator==(const char* lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) == 0;
  }

inline
bool
operator==(const char256& lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs) == 0;
  }

inline
bool
operator!=(const char256& lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) != 0;
  }

inline
bool
operator!=(const char* lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) != 0;
  }

inline
bool
operator!=(const char256& lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs) != 0;
  }

inline
bool
operator<(const char256& lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) < 0;
  }

inline
bool
operator<(const char* lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) < 0;
  }

inline
bool
operator<(const char256& lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs) < 0;
  }

inline
bool
operator>(const char256& lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) > 0;
  }

inline
bool
operator>(const char* lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) > 0;
  }

inline
bool
operator>(const char256& lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs) > 0;
  }

inline
bool
operator<=(const char256& lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) <= 0;
  }

inline
bool
operator<=(const char* lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) <= 0;
  }

inline
bool
operator<=(const char256& lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs) <= 0;
  }

inline
bool
operator>=(const char256& lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) >= 0;
  }

inline
bool
operator>=(const char* lhs, const char256& rhs) noexcept
  {
    return ::strcmp(lhs, rhs) >= 0;
  }

inline
bool
operator>=(const char256& lhs, const char* rhs) noexcept
  {
    return ::strcmp(lhs, rhs) >= 0;
  }

}  // namespace poseidon
#endif
