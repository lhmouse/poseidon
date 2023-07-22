// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_response_headers.hpp"
#include "../utils.hpp"
#include <http_parser.h>
namespace poseidon {

void
HTTP_Response_Headers::
encode(tinyfmt& fmt) const
  {
    // Write the status line. If `reason` is empty, a default reason string
    // is written. This function does not validate whether these fields
    // contain valid values.
    fmt << "HTTP/1.1 " << this->status << " ";

    if(this->reason.empty())
      fmt << ::http_status_str(static_cast<::http_status>(this->status));
    else
      fmt << this->reason;

    // Write response headers. Empty headers are ignored.
    for(const auto& hpair : this->headers) {
      if(hpair.first.empty())
        continue;

      fmt << "\r\n" << hpair.first << ": ";

      if(hpair.second.is_string())
        fmt << hpair.second.as_string();
      else
        fmt << hpair.second;
    }

    // Terminate the response with an empty line.
    fmt << "\r\n\r\n";
  }

}  // namespace poseidon
