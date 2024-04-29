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
    // Write the request line. If `method` is an empty string, `GET` is assumed.
    // This function does not validate whether these fields contain valid values.
    if(this->method_bytes[0] == 0)
      fmt << "GET ";
    else {
      alignas(16) char method_str[16];
      __m128i t = _mm_load_si128(&(this->packed_fields_1));
      t = _mm_and_si128(t, _mm_setr_epi32(-1, -1, -1, 0));
      _mm_store_si128(reinterpret_cast<__m128i*>(method_str), t);
      uint32_t len = static_cast<uint32_t>(_mm_cmpistri(t, _mm_setzero_si128(), 0x08));
      ::memcpy(method_str + len, " ", 2);
      fmt << method_str;
    }

    if(this->is_proxy) {
      // The URI shall be absolute.
      if(this->is_ssl)
        fmt << "https://";
      else
        fmt << "http://";

      if(this->uri_userinfo != "")
        fmt << this->uri_userinfo << '@';

      fmt << this->uri_host;
      if(this->uri_port != 0)
        fmt << ':' << this->uri_port;
    }

    if(this->uri_path[0] != '/')
      fmt << '/' << this->uri_path;
    else
      fmt << this->uri_path;

    if(this->uri_query != "")
      fmt << '?' << this->uri_query;

    fmt << " HTTP/1.1";

    // Write request headers. Empty headers are ignored.
    for(const auto& hpair : this->headers)
      if(hpair.first != "") {
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
