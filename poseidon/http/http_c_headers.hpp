// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_REQUEST_HEADERS_
#define POSEIDON_HTTP_HTTP_REQUEST_HEADERS_

#include "../fwd.hpp"
#include "http_field_name.hpp"
#include "http_value.hpp"
#include "enums.hpp"
namespace poseidon {

struct HTTP_C_Headers
  {
    union {
      __m128i packed_fields_1 = { };
      HTTP_Method method;
      struct {
        char method_str[8];
        char method_nul_do_not_use;
        uint8_t is_proxy : 1;
        uint8_t is_ssl : 1;
        uint16_t port;
        uint32_t reserved_1;
      };
    };

    cow_string raw_host;
    cow_string raw_userinfo;
    cow_string raw_path;
    cow_string raw_query;
    cow_bivector<HTTP_Field_Name, HTTP_Value> headers;

    HTTP_C_Headers() noexcept = default;
    HTTP_C_Headers(const HTTP_C_Headers&) = default;
    HTTP_C_Headers(HTTP_C_Headers&&) = default;
    HTTP_C_Headers& operator=(const HTTP_C_Headers&) & = default;
    HTTP_C_Headers& operator=(HTTP_C_Headers&&) & = default;
    ~HTTP_C_Headers();

    HTTP_C_Headers&
    swap(HTTP_C_Headers& other) noexcept
      {
        ::std::swap(this->packed_fields_1, other.packed_fields_1);
        this->raw_host.swap(other.raw_host);
        this->raw_userinfo.swap(other.raw_userinfo);
        this->raw_path.swap(other.raw_path);
        this->raw_query.swap(other.raw_query);
        this->headers.swap(other.headers);
        return *this;
      }

    // Clears all fields.
    void
    clear() noexcept
      {
        this->packed_fields_1 = _mm_setzero_si128();
        this->raw_host.clear();
        this->raw_userinfo.clear();
        this->raw_path.clear();
        this->raw_query.clear();
        this->headers.clear();
      }

    // Encodes an arbitrary path and assigns it to `raw_path`.
    void
    encode_and_set_path(chars_view path);

    // Encodes a key-value pair and appends it to `raw_query`.
    void
    encode_and_append_query(chars_view key, chars_view value);

    // Creates a `Host:` header if no one is found.
    void
    set_request_host(const Abstract_Socket& socket, const cow_string& default_host);

    // Encodes headers in wire format. Lines are separated by CR LF pairs. The
    // output will be suitable for sending through a stream socket.
    void
    encode(tinyfmt& fmt) const;
  };

inline
void
swap(HTTP_C_Headers& lhs, HTTP_C_Headers& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
