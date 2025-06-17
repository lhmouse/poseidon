// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_request_headers.hpp"
#include "../socket/abstract_socket.hpp"
#include "../utils.hpp"
namespace poseidon {

HTTP_Request_Headers::
~HTTP_Request_Headers()
  {
  }

void
HTTP_Request_Headers::
encode_and_set_path(chars_view path)
  {
    char seq[4] = "%";
    int dval;

    this->raw_path.clear();
    this->raw_path.push_back('/');

    for(size_t k = 0;  k != path.n;  ++k)
      switch(path.p[k])
        {
        case '0' ... '9':
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '-':
        case '_':
        case '~':
        case '.':
        case '/':
          // These characters are safe.
          this->raw_path.push_back(path.p[k]);
          break;

        default:
          // Encode this unsafe character.
          dval = static_cast<unsigned char>(path.p[k]) >> 4 & 0x0F;
          seq[1] = static_cast<char>(dval + '0' + ((9 - dval) >> 15 & 7));
          dval = static_cast<unsigned char>(path.p[k]) & 0x0F;
          seq[2] = static_cast<char>(dval + '0' + ((9 - dval) >> 15 & 7));
          this->raw_path.append(seq, 3);
          break;
        }
  }


void
HTTP_Request_Headers::
encode_and_append_query(chars_view key, chars_view value)
  {
    if(this->raw_query.size() != 0)
      this->raw_query.push_back('&');

    char seq[4] = "%";
    int dval;

    for(size_t k = 0;  k != key.n;  ++k)
      switch(key.p[k])
        {
        case '0' ... '9':
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '-':
        case '_':
        case '~':
        case '.':
        case '/':
          // These characters are safe.
          this->raw_query.push_back(key.p[k]);
          break;

        case ' ':
          // This is a special case.
          this->raw_query.push_back('+');
          break;

        default:
          // Encode this unsafe character.
          dval = static_cast<unsigned char>(key.p[k]) >> 4 & 0x0F;
          seq[1] = static_cast<char>(dval + '0' + ((9 - dval) >> 15 & 7));
          dval = static_cast<unsigned char>(key.p[k]) & 0x0F;
          seq[2] = static_cast<char>(dval + '0' + ((9 - dval) >> 15 & 7));
          this->raw_query.append(seq, 3);
          break;
        }

    this->raw_query.push_back('=');

    for(size_t k = 0;  k != value.n;  ++k)
      switch(value.p[k])
        {
        case '0' ... '9':
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '-':
        case '_':
        case '~':
        case '.':
        case '/':
        case '=':
          // These characters are safe.
          this->raw_query.push_back(value.p[k]);
          break;

        case ' ':
          // This is a special case.
          this->raw_query.push_back('+');
          break;

        default:
          // Encode this unsafe character.
          dval = static_cast<unsigned char>(value.p[k]) >> 4 & 0x0F;
          seq[1] = static_cast<char>(dval + '0' + ((9 - dval) >> 15 & 7));
          dval = static_cast<unsigned char>(value.p[k]) & 0x0F;
          seq[2] = static_cast<char>(dval + '0' + ((9 - dval) >> 15 & 7));
          this->raw_query.append(seq, 3);
          break;
        }
  }

void
HTTP_Request_Headers::
set_request_host(const Abstract_Socket& socket, const cow_string& default_host)
  {
    // A proxy request has a hostname in the URI.
    if(this->is_proxy)
      return;

    if(this->raw_host == "") {
      // Ensure `uri_host` is set to a non-empty string.
      if(default_host != "")
        this->raw_host = default_host;
      else
        this->raw_host = socket.remote_address().print_to_string();
    }

    // Replace the existent host header.
    size_t index = SIZE_MAX;
    for(size_t k = 0;  k != this->headers.size();  ++k)
      if(this->headers[k].first == "Host") {
        index = k;
        break;
      }

    if(index == SIZE_MAX)
      this->headers.emplace_back(&"Host", this->raw_host);
    else
      this->headers.mut(index).second = this->raw_host;
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

      if(this->raw_userinfo != "")
        fmt << this->raw_userinfo << '@';

      fmt << this->raw_host;

      if(this->port != 0)
        fmt << ':' << this->port;
    }

    if(this->raw_path[0] != '/')
      fmt << '/' << this->raw_path;
    else
      fmt << this->raw_path;

    if(this->raw_query != "")
      fmt << '?' << this->raw_query;

    fmt << " HTTP/1.1";

    // Write request headers. Empty headers are ignored.
    for(const auto& hr : this->headers)
      if(hr.first != "")
        fmt << "\r\n" << hr.first << ": " << hr.second.as_string();

    // Terminate the request with an empty line.
    fmt << "\r\n\r\n";
  }

}  // namespace poseidon
