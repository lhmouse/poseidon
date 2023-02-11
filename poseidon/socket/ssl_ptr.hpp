// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_SSL_PTR_
#define POSEIDON_SOCKET_SSL_PTR_

#include "../fwd.hpp"
#include <openssl/ossl_typ.h>
extern "C" int SSL_up_ref(SSL*) __attribute__((__nothrow__));
extern "C" void SSL_free(SSL*) __attribute__((__nothrow__));
namespace poseidon {

class SSL_ptr
  {
  private:
    ::SSL* m_ptr;

  public:
    constexpr
    SSL_ptr(::SSL* ptr = nullptr) noexcept
      : m_ptr(ptr)  { }

    SSL_ptr(const SSL_ptr& other) noexcept
      : m_ptr(other.do_up_ref())  { }

    SSL_ptr(SSL_ptr&& other) noexcept
      : m_ptr(other.release())  { }

    SSL_ptr&
    operator=(const SSL_ptr& other) & noexcept
      { return this->reset(other.do_up_ref());  }

    SSL_ptr&
    operator=(SSL_ptr&& other) & noexcept
      { return this->reset(other.release());  }

    ~SSL_ptr()
      { this->do_free();  }

  private:
    ::SSL*
    do_up_ref() const noexcept
      {
        if(this->m_ptr)
          ::SSL_up_ref(this->m_ptr);

        return this->m_ptr;
      }

    void
    do_free() noexcept
      {
        if(this->m_ptr)
          ::SSL_free(this->m_ptr);

#ifdef ROCKET_DEBUG
        this->m_ptr = (::SSL*) 0xDEADBEEF;
#endif  // DEBUG
      }

  public:
    explicit constexpr operator
    bool() const noexcept
      { return this->m_ptr != nullptr;  }

    constexpr operator
    ::SSL*() const noexcept
      { return this->m_ptr;  }

    constexpr
    ::SSL*
    get() const noexcept
      { return this->m_ptr;  }

    ::SSL*
    release() noexcept
      {
        return ::std::exchange(this->m_ptr, nullptr);
      }

    SSL_ptr&
    reset(::SSL* ptr = nullptr) noexcept
      {
        this->do_free();
        this->m_ptr = ptr;
        return *this;
      }

    SSL_ptr&
    swap(SSL_ptr& other) noexcept
      {
        ::std::swap(this->m_ptr, other.m_ptr);
        return *this;
      }
  };

inline
void
swap(SSL_ptr& lhs, SSL_ptr& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace poseidon
#endif
