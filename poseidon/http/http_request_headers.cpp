// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_request_headers.hpp"
#include "../utils.hpp"
namespace poseidon {

void
HTTP_Request_Headers::
encode(tinyfmt& fmt) const
  {
    // Write the request line. This function does not validate whether these
    // fields contain valid values.
    fmt << this->method << " " << this->uri << " HTTP/1.1";

    // Write request headers. Empty headers are ignored.
    for(const auto& hpair : this->headers) {
      if(hpair.first.empty())
        continue;

      fmt << "\r\n" << hpair.first << ": ";
      if(hpair.second.is_string())
        fmt << hpair.second.as_string();
      else
        fmt << hpair.second;
    }

    // Terminate the request with an empty line.
    fmt << "\r\n\r\n";
  }

}  // namespace poseidon
