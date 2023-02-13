// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_OPENSSL_FWD_
#define POSEIDON_THIRD_OPENSSL_FWD_

#include "../fwd.hpp"
#include <openssl/ossl_typ.h>
namespace poseidon {

template<class SSLxT, int uprefT(SSLxT*), void freeT(SSLxT*)>
class openssl_ptr
  {
  private:
    SSLxT* m_ptr;

  public:
    constexpr
    openssl_ptr() noexcept
      : m_ptr(nullptr)  { }

    constexpr
    openssl_ptr(SSLxT* ptr) noexcept
      : m_ptr(ptr)  { }

    openssl_ptr(const openssl_ptr& other) noexcept
      : m_ptr(other.do_up_ref())  { }

    openssl_ptr(openssl_ptr&& other) noexcept
      : m_ptr(other.release())  { }

    openssl_ptr&
    operator=(const openssl_ptr& other) & noexcept
      { return this->reset(other.do_up_ref());  }

    openssl_ptr&
    operator=(openssl_ptr&& other) & noexcept
      { return this->reset(other.release());  }

    ~openssl_ptr()
      { this->do_free();  }

  private:
    SSLxT*
    do_up_ref() const noexcept
      {
        if(this->m_ptr == nullptr)
          return nullptr;

        int r = uprefT(this->m_ptr);
        ROCKET_ASSERT(r == 1);
        return this->m_ptr;
      }

    void
    do_free() noexcept
      {
        if(this->m_ptr == nullptr)
          return;

        freeT(this->m_ptr);
        ROCKET_ASSERT(!!(this->m_ptr = (SSLxT*) 0xDEADBEEF));
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

    openssl_ptr&
    swap(openssl_ptr& other) noexcept
      {
        ::std::swap(this->m_ptr, other.m_ptr);
        return *this;
      }
  };

template<class SSLxT, int uprefT(SSLxT*), void freeT(SSLxT*)>
inline
void
swap(openssl_ptr<SSLxT, uprefT, freeT>& lhs, openssl_ptr<SSLxT, uprefT, freeT>& rhs) noexcept
  {
    lhs.swap(rhs);
  }

extern "C" int BIO_up_ref(::BIO* a);
extern "C" void BIO_free(::BIO* a);
using BIO_ptr = openssl_ptr<::BIO, BIO_up_ref, BIO_free>;

extern "C" int ENGINE_up_ref(::ENGINE* e);
extern "C" void ENGINE_free(::ENGINE* e);
using ENGINE_ptr = openssl_ptr<::ENGINE, ENGINE_up_ref, ENGINE_free>;

extern "C" int SSL_CTX_up_ref(::SSL_CTX* ctx);
extern "C" void SSL_CTX_free(::SSL_CTX* ctx);
using SSL_CTX_ptr = openssl_ptr<::SSL_CTX, SSL_CTX_up_ref, SSL_CTX_free>;

extern "C" int SSL_up_ref(::SSL* s);
extern "C" void SSL_free(::SSL* s);
using SSL_ptr = openssl_ptr<::SSL, SSL_up_ref, SSL_free>;

extern "C" int EC_KEY_up_ref(::EC_KEY* key);
extern "C" void EC_KEY_free(::EC_KEY* key);
using EC_KEY_ptr = openssl_ptr<::EC_KEY, EC_KEY_up_ref, EC_KEY_free>;

extern "C" int EVP_PKEY_up_ref(::EVP_PKEY* key);
extern "C" void EVP_PKEY_free(::EVP_PKEY* key);
using EVP_PKEY_ptr = openssl_ptr<::EVP_PKEY, EVP_PKEY_up_ref, EVP_PKEY_free>;

extern "C" int X509_STORE_up_ref(::X509_STORE* v);
extern "C" void X509_STORE_free(::X509_STORE* v);
using X509_STORE_ptr = openssl_ptr<::X509_STORE, X509_STORE_up_ref, X509_STORE_free>;

extern "C" int X509_up_ref(::X509* a);
extern "C" void X509_free(::X509* a);
using X509_ptr = openssl_ptr<::X509, X509_up_ref, X509_free>;

} // namespace poseidon
#endif
