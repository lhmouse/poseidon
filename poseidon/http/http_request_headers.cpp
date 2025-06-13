// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_request_headers.hpp"
#include "../socket/abstract_socket.hpp"
#include "../utils.hpp"
namespace poseidon {

void
HTTP_Request_Headers::
set_request_host(const Abstract_Socket& socket, const cow_string& default_host)
  {
    // A proxy request has a hostname in the URI.
    if(this->is_proxy)
      return;

    if(this->uri_host == "") {
      // Ensure `uri_host` is set to a non-empty string.
      if(default_host != "")
        this->uri_host = default_host;
      else
        this->uri_host = socket.remote_address().print_to_string();
    }

    // Replace the existent host header.
    size_t index = SIZE_MAX;
    for(size_t k = 0;  k != this->headers.size();  ++k)
      if(this->headers[k].first == "Host") {
        index = k;
        break;
      }

    if(index == SIZE_MAX)
      this->headers.emplace_back(&"Host", this->uri_host);
    else
      this->headers.mut(index).second = this->uri_host;
  }

void
HTTP_Request_Headers::
encode(tinyfmt& fmt) const
  {
    // Write the request line. If `method` is an empty string, `GET` is assumed.
    // This function does not validate whether these fields contain valid values.
    if(this->method_str[0] == 0)
      fmt << "GET ";
    else {
      const __m128i zr = _mm_setzero_si128();
      alignas(16) char str[16];
      __m128i t = _mm_load_si128(&(this->packed_fields_1));
      t = _mm_blend_epi16(t, zr, 0xC0);
      _mm_store_si128(reinterpret_cast<__m128i*>(str), t);
      ::memcpy(str + static_cast<uint32_t>(_mm_cmpistri(t, zr, 0x08)), " ", 2);
      fmt << str;
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
    for(const auto& hr : this->headers)
      if(hr.first != "") {
        fmt << "\r\n" << hr.first << ": ";
        if(hr.second.is_string())
          fmt << hr.second.as_string();
        else
          fmt << hr.second;
      }

    // Terminate the request with an empty line.
    fmt << "\r\n\r\n";
  }

}  // namespace poseidon
