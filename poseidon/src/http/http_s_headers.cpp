// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/http_s_headers.hpp"
#include "../../utils.hpp"
namespace poseidon {

shallow_string
HTTP_S_Headers::
default_reason(HTTP_Status status)
  noexcept
  {
    switch(status)
      {
      case http_status_continue:
        return &"Continue";

      case http_status_switching_protocols:
        return &"Switching Protocols";

      case http_status_ok:
        return &"OK";

      case http_status_created:
        return &"Created";

      case http_status_accepted:
        return &"Accepted";

      case http_status_nonauthoritative_information:
        return &"Non-Authoritative Information";

      case http_status_no_content:
        return &"No Content";

      case http_status_reset_content:
        return &"Reset Content";

      case http_status_partial_content:
        return &"Partial Content";

      case http_status_multiple_choices:
        return &"Multiple Choices";

      case http_status_moved_permanently:
        return &"Moved Permanently";

      case http_status_found:
        return &"Found";

      case http_status_see_other:
        return &"See Other";

      case http_status_not_modified:
        return &"Not Modified";

      case http_status_use_proxy:
        return &"Use Proxy";

      case http_status_temporary_redirect:
        return &"Temporary Redirect";

      case http_status_bad_request:
        return &"Bad Request";

      case http_status_unauthorized:
        return &"Unauthorized";

      case http_status_payment_required:
        return &"Payment Required";

      case http_status_forbidden:
        return &"Forbidden";

      case http_status_not_found:
        return &"Not Found";

      case http_status_method_not_allowed:
        return &"Method Not Allowed";

      case http_status_not_acceptable:
        return &"Not Acceptable";

      case http_status_proxy_authentication_required:
        return &"Proxy Authentication Required";

      case http_status_request_timeout:
        return &"Request Timeout";

      case http_status_conflict:
        return &"Conflict";

      case http_status_gone:
        return &"Gone";

      case http_status_length_required:
        return &"Length Required";

      case http_status_precondition_failed:
        return &"Precondition Failed";

      case http_status_payload_too_large:
        return &"Content Too Large";

      case http_status_uri_too_long:
        return &"URI Too Long";

      case http_status_unsupported_media_type:
        return &"Unsupported Media Type";

      case http_status_range_not_satisfiable:
        return &"Range Not Satisfiable";

      case http_status_expectation_failed:
        return &"Expectation Failed";

      case http_status_upgrade_required:
        return &"Upgrade Required";

      case http_status_internal_server_error:
        return &"Internal Server Error";

      case http_status_not_implemented:
        return &"Not Implemented";

      case http_status_bad_gateway:
        return &"Bad Gateway";

      case http_status_service_unavailable:
        return &"Service Unavailable";

      case http_status_gateway_timeout:
        return &"Gateway Timeout";

      case http_status_http_version_not_supported:
        return &"HTTP Version Not Supported";

      default:
        return &"Unknown Status";
      }
  }

HTTP_S_Headers::
~HTTP_S_Headers()
  {
  }

size_t
HTTP_S_Headers::
estimate_size()
  const
  {
    size_t tlen = 12 + this->reason.size();
    for(const auto& hr : this->headers)
      tlen += 4 + hr.first.length() + hr.second.as_string_length();
    return tlen;
  }

void
HTTP_S_Headers::
encode(tinyfmt& fmt)
  const
  {
    // Write the status line. If `reason` is empty, a default reason string
    // is written. This function does not validate whether these fields
    // contain valid values.
    fmt << "HTTP/1.1 " << this->status << " ";
    if(this->reason.size() != 0)
      fmt << this->reason;
    else
      fmt << HTTP_S_Headers::default_reason(this->status);

    // Write response headers. Empty headers are ignored.
    for(const auto& hr : this->headers)
      if(hr.first != "")
        fmt << "\r\n" << hr.first << ": " << hr.second.as_string();

    // Terminate the response with an empty line.
    fmt << "\r\n\r\n";
  }

}  // namespace poseidon
