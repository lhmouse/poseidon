// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_REQUEST_HEADERS_
#define POSEIDON_HTTP_HTTP_REQUEST_HEADERS_

#include "../fwd.hpp"
#include "http_field_name.hpp"
#include "http_value.hpp"
#include "enums.hpp"
namespace poseidon {

struct HTTP_Request_Headers
  {
    union {
      __m128i packed_fields_1 = { };
      HTTP_Method method;
      struct {
        char method_str[8];
        char method_nul_do_not_use;
        uint8_t is_proxy : 1;
        uint8_t is_ssl : 1;
        uint16_t uri_port;
        uint32_t reserved_1;
      };
    };

    cow_string uri_host;
    cow_string uri_userinfo;
    cow_string uri_path;
    cow_string uri_query;
    cow_bivector<HTTP_Field_Name, HTTP_Value> headers;

    HTTP_Request_Headers&
    swap(HTTP_Request_Headers& other) noexcept
      {
        ::std::swap(this->packed_fields_1, other.packed_fields_1);
        this->uri_host.swap(other.uri_host);
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
        this->packed_fields_1 = _mm_setzero_si128();
        this->uri_host.clear();
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
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
