// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_OPENSSL_FWD_
#define POSEIDON_THIRD_OPENSSL_FWD_

#include "../fwd.hpp"
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
namespace poseidon {

#define POSEIDON_OPENSSL_FWD_DEFINE_PTR_(OBJ)  \
  \
  class OBJ##_ptr  \
    {  \
    private:  \
      ::OBJ* m_ptr;  \
  \
    public:  \
      constexpr  \
      OBJ##_ptr(nullptr_t = nullptr) noexcept  \
        :  \
          m_ptr(nullptr)  \
        {  \
        }  \
  \
      explicit constexpr  \
      OBJ##_ptr(::OBJ* ptr) noexcept  \
        :  \
          m_ptr(ptr)  \
        {  \
        }  \
  \
      OBJ##_ptr(const OBJ##_ptr& other) noexcept  \
        {  \
          if(other.m_ptr) OBJ##_up_ref(other.m_ptr);  \
          this->m_ptr = other.m_ptr;  \
        }  \
  \
      OBJ##_ptr(OBJ##_ptr&& other) noexcept  \
        {  \
          this->m_ptr = other.release();  \
        }  \
  \
      OBJ##_ptr&  \
      operator=(nullptr_t) & noexcept  \
        {  \
          this->reset(nullptr);  \
          return *this;  \
        }  \
  \
      OBJ##_ptr&  \
      operator=(const OBJ##_ptr& other) & noexcept  \
        {  \
          if(other.m_ptr) OBJ##_up_ref(other.m_ptr);  \
          this->reset(other.m_ptr);  \
          return *this;  \
        }  \
  \
      OBJ##_ptr&  \
      operator=(OBJ##_ptr&& other) & noexcept  \
        {  \
          this->reset(other.release());  \
          return *this;  \
        }  \
  \
      OBJ##_ptr&  \
      swap(OBJ##_ptr& other) noexcept  \
        {  \
          ::std::swap(this->m_ptr, other.m_ptr);  \
          return *this;  \
        }  \
  \
      ~OBJ##_ptr()  \
        {  \
          if(this->m_ptr) OBJ##_free(this->m_ptr);  \
          this->m_ptr = (::OBJ*) -0xDEADBEEF;  \
        }  \
  \
    public:  \
      explicit constexpr operator  \
      bool() const noexcept  \
        { return this->m_ptr != nullptr;  }  \
  \
      constexpr operator  \
      ::OBJ*() const noexcept  \
        { return this->m_ptr;  }  \
  \
      constexpr  \
      ::OBJ*  \
      get() const noexcept  \
        { return this->m_ptr;  }  \
  \
      OBJ##_ptr&  \
      reset(::OBJ* ptr = nullptr) noexcept  \
        {  \
          if(this->m_ptr) OBJ##_free(this->m_ptr);  \
          this->m_ptr = ptr;  \
          return *this;  \
        }  \
  \
      ::OBJ*  \
      release() noexcept  \
        {  \
          ::OBJ* old_ptr = this->m_ptr;  \
          this->m_ptr = nullptr;  \
          return old_ptr;  \
        }  \
    };  \
  \
  inline  \
  void  \
  swap(OBJ##_ptr& lhs, OBJ##_ptr& rhs) noexcept  \
    {  \
      lhs.swap(rhs);  \
    }  \
  \
  class OBJ##_ptr  // no semicolon

POSEIDON_OPENSSL_FWD_DEFINE_PTR_(BIO);      // class BIO_ptr
POSEIDON_OPENSSL_FWD_DEFINE_PTR_(X509);     // class X509_ptr
POSEIDON_OPENSSL_FWD_DEFINE_PTR_(SSL_CTX);  // class SSL_CTX_ptr
POSEIDON_OPENSSL_FWD_DEFINE_PTR_(SSL);      // class SSL_ptr

}  // namespace poseidon
#endif
