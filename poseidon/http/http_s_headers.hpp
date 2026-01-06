// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_RESPONSE_HEADERS_
#define POSEIDON_HTTP_HTTP_RESPONSE_HEADERS_

#include "../fwd.hpp"
#include "http_field_name.hpp"
#include "http_value.hpp"
#include "enums.hpp"
namespace poseidon {

struct HTTP_S_Headers
  {
    HTTP_Status status = http_status_null;
    uint16_t reserved_1 = 0;
    uint32_t reserved_2 = 0;
    cow_string reason;
    cow_bivector<HTTP_Field_Name, HTTP_Value> headers;

    HTTP_S_Headers() noexcept = default;
    HTTP_S_Headers(const HTTP_S_Headers&) = default;
    HTTP_S_Headers(HTTP_S_Headers&&) = default;
    HTTP_S_Headers& operator=(const HTTP_S_Headers&) & = default;
    HTTP_S_Headers& operator=(HTTP_S_Headers&&) & = default;
    ~HTTP_S_Headers();

    HTTP_S_Headers&
    swap(HTTP_S_Headers& other)
      noexcept
      {
        ::std::swap(this->status, other.status);
        this->reason.swap(other.reason);
        this->headers.swap(other.headers);
        return *this;
      }

    // Clears all fields.
    void
    clear()
      noexcept
      {
        this->status = http_status_null;
        this->reason.clear();
        this->headers.clear();
      }

    // Encodes headers in wire format. Lines are separated by CR LF pairs. The
    // output will be suitable for sending through a stream socket.
    void
    encode(tinyfmt& fmt)
      const;
  };

inline
void
swap(HTTP_S_Headers& lhs, HTTP_S_Headers& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
