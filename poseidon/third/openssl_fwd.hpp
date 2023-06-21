// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_OPENSSL_FWD_
#define POSEIDON_THIRD_OPENSSL_FWD_

#include "../fwd.hpp"
#include <openssl/ossl_typ.h>
namespace poseidon {

template<class SSLxT>
struct openssl_ptr_traits;

template<class SSLxT>
class openssl_ptr
  {
  private:
    SSLxT* m_ptr;

  public:
    constexpr
    openssl_ptr() noexcept
      : m_ptr(nullptr)
      { }

    constexpr
    openssl_ptr(SSLxT* ptr) noexcept
      : m_ptr(ptr)
      { }

    openssl_ptr(const openssl_ptr& other) noexcept
      : m_ptr(other.do_up_ref())
      { }

    openssl_ptr(openssl_ptr&& other) noexcept
      : m_ptr(other.release())
      { }

    openssl_ptr&
    operator=(const openssl_ptr& other) & noexcept
      { return this->reset(other.do_up_ref());  }

    openssl_ptr&
    operator=(openssl_ptr&& other) & noexcept
      { return this->reset(other.release());  }

    openssl_ptr&
    swap(openssl_ptr& other) noexcept
      {
        ::std::swap(this->m_ptr, other.m_ptr);
        return *this;
      }

    ~openssl_ptr()
      { this->do_free();  }

  private:
    SSLxT*
    do_up_ref() const noexcept
      {
        if(this->m_ptr == nullptr)
          return nullptr;

        int r = openssl_ptr_traits<SSLxT>::up_ref(this->m_ptr);
        ROCKET_ASSERT(r == 1);
        return this->m_ptr;
      }

    void
    do_free() noexcept
      {
        if(this->m_ptr == nullptr)
          return;

        openssl_ptr_traits<SSLxT>::free(this->m_ptr);
        this->m_ptr = (SSLxT*) -0xDEADBEEF;
      }

  public:
    explicit constexpr operator
    bool() const noexcept
      { return this->m_ptr != nullptr;  }

    constexpr operator
    SSLxT*() const noexcept
      { return this->m_ptr;  }

    constexpr
    SSLxT*
    get() const noexcept
      { return this->m_ptr;  }

    openssl_ptr&
    reset(SSLxT* ptr = nullptr) noexcept
      {
        this->do_free();
        this->m_ptr = ptr;
        return *this;
      }

    SSLxT*
    release() noexcept
      {
        return ::std::exchange(this->m_ptr, nullptr);
      }
  };

template<class SSLxT>
inline
void
swap(openssl_ptr<SSLxT>& lhs, openssl_ptr<SSLxT>& rhs) noexcept
  {
    lhs.swap(rhs);
  }

#define POSEIDON_OPENSSL_FWD_DEFINE_PTR_(OBJ, xFree)  \
  extern "C" int OBJ##_up_ref(::OBJ* a) __attribute__((__nothrow__));  \
  extern "C" xFree OBJ##_free(::OBJ* a) __attribute__((__nothrow__));  \
  template<>  \
  struct openssl_ptr_traits<::OBJ>  \
    {  \
      static int up_ref(::OBJ* a) noexcept { return OBJ##_up_ref(a);  }  \
      static void free(::OBJ* a) noexcept { OBJ##_free(a);  }  \
    };  \
  using OBJ##_ptr = openssl_ptr<::OBJ>;

POSEIDON_OPENSSL_FWD_DEFINE_PTR_(BIO, int)          // class BIO_ptr
POSEIDON_OPENSSL_FWD_DEFINE_PTR_(X509_STORE, void)  // class X509_STORE_ptr
POSEIDON_OPENSSL_FWD_DEFINE_PTR_(X509, void)        // class X509_ptr
POSEIDON_OPENSSL_FWD_DEFINE_PTR_(SSL_CTX, void)     // class SSL_CTX_ptr
POSEIDON_OPENSSL_FWD_DEFINE_PTR_(SSL, void)         // class SSL_ptr

} // namespace poseidon
#endif
