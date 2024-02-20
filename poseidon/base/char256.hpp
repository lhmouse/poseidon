// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_CHAR256_
#define POSEIDON_BASE_CHAR256_

#include "../fwd.hpp"
namespace poseidon {

class char256
  {
  private:
    union {
      char m_data[4];
      ::std::aligned_storage<256>::type m_stor;
    };

  public:
    // Constructs a null-terminated string of zero characters.
    // This constructor is not explicit as it doesn't allocate memory.
    constexpr char256() noexcept
      :
        m_data()
      { }

    // Constructs a null-terminated string.
    // This constructor is not explicit as it doesn't allocate memory.
    constexpr char256(const char* str_opt)
      :
        m_data()
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
tinyfmt&
operator<<(tinyfmt& fmt, const char256& cbuf)
  { return fmt.putn(cbuf.c_str(), ::rocket::xstrlen(cbuf.c_str()));  }

}  // namespace poseidon
#endif
