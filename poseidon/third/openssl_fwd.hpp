// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_OPENSSL_FWD_
#define POSEIDON_THIRD_OPENSSL_FWD_

#include "../fwd.hpp"
#include <openssl/bio.h>
#include <openssl/ssl.h>
namespace poseidon {

struct BIO_deleter
  {
    void
    operator()(::BIO* p) const noexcept
      { ::BIO_free(p);  }
  };

struct SSL_deleter
  {
    void
    operator()(::SSL* p) const noexcept
      { ::SSL_free(p);  }
  };

struct SSL_CTX_deleter
  {
    void
    operator()(::SSL_CTX* p) const noexcept
      { ::SSL_CTX_free(p);  }
  };

using uni_BIO = ::rocket::unique_ptr<::BIO, BIO_deleter>;
using uni_SSL = ::rocket::unique_ptr<::SSL, SSL_deleter>;
using uni_SSL_CTX = ::rocket::unique_ptr<::SSL_CTX, SSL_CTX_deleter>;

}  // namespace poseidon
#endif
