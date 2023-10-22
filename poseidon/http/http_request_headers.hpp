// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_REQUEST_HEADERS_
#define POSEIDON_HTTP_HTTP_REQUEST_HEADERS_

#include "../fwd.hpp"
#include "http_value.hpp"
namespace poseidon {

struct HTTP_Request_Headers
  {
    const char* method;  // null implies `GET`
    cow_string uri_host;
    uint16_t uri_port;  // zero implies `80` or `443`, basing on `is_ssl`
    bool is_proxy;
    bool is_ssl;
    cow_string uri_userinfo;
    cow_string uri_path;
    cow_string uri_query;
    cow_bivector<cow_string, HTTP_Value> headers;

    // Define some helper functions.
    constexpr
    HTTP_Request_Headers() noexcept
      :
        method(), uri_host(), uri_port(), is_proxy(), is_ssl(), headers()
      { }

    HTTP_Request_Headers&
    swap(HTTP_Request_Headers& other) noexcept
      {
        ::std::swap(this->method, other.method);
        this->uri_host.swap(other.uri_host);
        ::std::swap(this->uri_port, other.uri_port);
        ::std::swap(this->is_proxy, other.is_proxy);
        ::std::swap(this->is_ssl, other.is_ssl);
        this->uri_userinfo.swap(other.uri_userinfo);
        this->uri_path.swap(other.uri_path);
        this->uri_query.swap(other.uri_query);
        this->headers.swap(other.headers);
        return *this;
      }

    // Clears all fields.
    void
    clear() noexcept
      {
        this->method = nullptr;
        this->uri_host.clear();
        this->uri_port = 0;
        this->is_proxy = false;
        this->is_ssl = false;
        this->uri_userinfo.clear();
        this->uri_path.clear();
        this->uri_query.clear();
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
