// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_REQUEST_HEADERS_
#define POSEIDON_HTTP_HTTP_REQUEST_HEADERS_

#include "../fwd.hpp"
#include "http_value.hpp"
namespace poseidon {

struct HTTP_Request_Headers
  {
    cow_string method;
    cow_string uri;
    cow_bivector<cow_string, HTTP_Value> headers;

    // Define some helper functions.
    constexpr
    HTTP_Request_Headers() noexcept
      : method(), uri(), headers()  { }

    HTTP_Request_Headers&
    swap(HTTP_Request_Headers& other) noexcept
      {
        this->method.swap(other.method);
        this->uri.swap(other.uri);
        this->headers.swap(other.headers);
        return *this;
      }

    // Clears all fields.
    void
    clear() noexcept
      {
        this->method.clear();
        this->uri.clear();
        this->headers.clear();
      }

    // Encodes headers in wire format. Lines are separated by CR LF pairs. The
    // output will be suitable for sending through a stream socket.
    void
    encode(tinyfmt& fmt) const;
  };

inline
void
swap(HTTP_Request_Headers& lhs, HTTP_Request_Headers& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace poseidon
#endif
