// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_OPENSSL_FWD_
#define POSEIDON_THIRD_OPENSSL_FWD_

#include "../fwd.hpp"
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
namespace poseidon {

#define POSEIDON_OPENSSL_shared_(OBJ)  \
  \
  class shared_##OBJ  \
    {  \
    private:  \
      ::OBJ* m_ptr;  \
  \
    public:  \
      constexpr  \
      shared_##OBJ(nullptr_t = nullptr) noexcept  \
        :  \
          m_ptr(nullptr)  \
        { }  \
  \
      explicit constexpr  \
      shared_##OBJ(::OBJ* ptr) noexcept  \
        :  \
          m_ptr(ptr)  \
        { }  \
  \
      shared_##OBJ(const shared_##OBJ& other) noexcept  \
        {  \
          if(other.m_ptr) OBJ##_up_ref(other.m_ptr);  \
          this->m_ptr = other.m_ptr;  \
        }  \
  \
      shared_##OBJ(shared_##OBJ&& other) noexcept  \
        {  \
          this->m_ptr = other.release();  \
        }  \
  \
      shared_##OBJ&  \
      operator=(nullptr_t) & noexcept  \
        {  \
          this->reset(nullptr);  \
          return *this;  \
        }  \
  \
      shared_##OBJ&  \
      operator=(const shared_##OBJ& other) & noexcept  \
        {  \
          if(other.m_ptr) OBJ##_up_ref(other.m_ptr);  \
          this->reset(other.m_ptr);  \
          return *this;  \
        }  \
  \
      shared_##OBJ&  \
      operator=(shared_##OBJ&& other) & noexcept  \
        {  \
          this->reset(other.release());  \
          return *this;  \
        }  \
  \
      shared_##OBJ&  \
      swap(shared_##OBJ& other) noexcept  \
        {  \
          ::std::swap(this->m_ptr, other.m_ptr);  \
          return *this;  \
        }  \
  \
      ~shared_##OBJ()  \
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
      shared_##OBJ&  \
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
  swap(shared_##OBJ& lhs, shared_##OBJ& rhs) noexcept  \
    { lhs.swap(rhs);  }  \
  \
  class shared_##OBJ  // no semicolon

POSEIDON_OPENSSL_shared_(SSL);  // shared_SSL
POSEIDON_OPENSSL_shared_(BIO);  // shared_BIO
POSEIDON_OPENSSL_shared_(X509);  // shared_X509
POSEIDON_OPENSSL_shared_(SSL_CTX);  // shared_SSL_CTX

}  // namespace poseidon
#endif
