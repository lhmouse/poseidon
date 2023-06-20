// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_response_headers.hpp"
#include "../utils.hpp"
#include <http_parser.h>
namespace poseidon {

HTTP_Response_Headers::
~HTTP_Response_Headers()
  {
  }

tinyfmt&
HTTP_Response_Headers::
print(tinyfmt& fmt) const
  {
    // `HTTP/1.1 400 Bad Request`
    if(this->m_reason.empty())
      fmt << "HTTP/1.1 " << this->m_status << " " << ::http_status_str((::http_status) this->m_status) << "\r\n";
    else
      fmt << "HTTP/1.1 " << this->m_status << " " << this->m_reason << "\r\n";

    // `Server: test`
    // `Content-Length: 42`
    for(const auto& r : this->m_headers)
      if(r.second.is_string())
        fmt << r.first << ": " << r.second.as_string() << "\r\n";
      else
        fmt << r.first << ": " << r.second << "\r\n";

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
