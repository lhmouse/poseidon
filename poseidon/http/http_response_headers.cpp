// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_response_headers.hpp"
#include "../utils.hpp"
#include <http_parser.h>
namespace poseidon {

tinyfmt&
HTTP_Response_Headers::
print(tinyfmt& fmt) const
  {
    // `HTTP/1.1 400 Bad Request`
    fmt << "HTTP/1.1 " << this->status << " ";

    if(this->reason.empty())
      fmt << ::http_status_str((::http_status) this->status) << "\r\n";
    else
      fmt << this->reason << "\r\n";

    // `Server: test`
    // `Content-Length: 42`
    for(const auto& r : this->headers) {
      // Ignore empty names, making it easier to use.
      if(r.first.empty())
        continue;

      fmt << r.first << ": ";

      if(r.second.is_string())
        fmt << r.second.as_string() << "\r\n";
      else
        fmt << r.second << "\r\n";
    }

    // Terminate the header with an empty line.
    fmt << "\r\n";
    return fmt;
  }

cow_string
HTTP_Response_Headers::
print_to_string() const
  {
    ::rocket::tinyfmt_str fmt;
    this->print(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
