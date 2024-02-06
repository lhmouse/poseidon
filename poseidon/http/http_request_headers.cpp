// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_request_headers.hpp"
#include "../utils.hpp"
namespace poseidon {

void
HTTP_Request_Headers::
encode(tinyfmt& fmt) const
  {
    // Write the request line. If `method` is null or points to an empty string,
    // `GET` is assumed. This function does not validate whether these fields
    // contain valid values.
    if((this->method == nullptr) || (this->method[0] == 0))
      fmt << "GET ";
    else
      fmt << this->method << ' ';

    if(this->is_proxy) {
      // Initiate an absolute URI.
      if(this->is_ssl)
        fmt << "https://";
      else
        fmt << "http://";

      if(!this->uri_userinfo.empty())
        fmt << this->uri_userinfo << '@';

      fmt << this->uri_host;

      if(this->uri_port != 0)
        fmt << ':' << this->uri_port;
    }

    if(!this->uri_path.starts_with("/"))
      fmt << '/';

    fmt << this->uri_path;

    if(!this->uri_query.empty())
      fmt << '?' << this->uri_query;

    fmt << " HTTP/1.1";

    // Write request headers. Empty headers are ignored.
    for(const auto& hpair : this->headers)
      if(!hpair.first.empty()) {
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
