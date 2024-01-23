// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_OPENSSL_FWD_
#define POSEIDON_THIRD_OPENSSL_FWD_

#include "../fwd.hpp"
#include <openssl/bio.h>
#include <openssl/ssl.h>
namespace poseidon {

// class shared_SSL
using ::SSL;
POSEIDON_DEFINE_shared(SSL, ::SSL_up_ref, ::SSL_free);

// class shared_BIO
using ::BIO;
POSEIDON_DEFINE_shared(BIO, ::BIO_up_ref, ::BIO_free);

// class shared_SSL_CTX
using ::SSL_CTX;
POSEIDON_DEFINE_shared(SSL_CTX, ::SSL_CTX_up_ref, ::SSL_CTX_free);

}  // namespace poseidon
#endif
