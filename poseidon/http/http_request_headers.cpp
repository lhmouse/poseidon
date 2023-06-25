// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "http_request_headers.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Request_Headers::
~HTTP_Request_Headers()
  {
  }

tinyfmt&
HTTP_Request_Headers::
print(tinyfmt& fmt) const
  {
    // `POST /index.php HTTP/1.1`
    fmt << this->m_verb << " " << this->m_uri << " HTTP/1.1\r\n";

    // `User-Agent: test (foo)`
    // `Content-Length: 42`
    for(const auto& r : this->m_headers) {
      // Ignore empty names, making it easier to use.
      if(r.first.empty())
        continue;

      fmt << r.first << ": ";

      if(r.second.is_string())
        fmt << r.second.as_string() << "r\n";
      else
        fmt << r.second << "\r\n";
    }

    // Terminate the header with an empty line.
    fmt << "\r\n";
    return fmt;
  }

cow_string
HTTP_Request_Headers::
print_to_string() const
  {
    ::rocket::tinyfmt_str fmt;
    this->print(fmt);
    return fmt.extract_string();
  }

}  // namespace poseidon
