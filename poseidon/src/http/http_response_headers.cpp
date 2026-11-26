// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "../../http/http_response_headers.hpp"
#include "../../utils.hpp"
namespace poseidon {

shallow_string
HTTP_Response_Headers::
default_reason(HTTP_Status status) noexcept
  {
    switch(status)
      {
      case http_status_null:
        return &"No Status";

      case http_status_continue:
        return &"Continue";

      case http_status_switching_protocols:
        return &"Switching Protocols";

      case http_status_processing:
        return &"Processing";

      case http_status_early_hints:
        return &"Early Hints";

      case http_status_response_is_stale:
        return &"Response is Stale";

      case http_status_revalidation_failed:
        return &"Revalidation Failed";

      case http_status_disconnected_operation:
        return &"Disconnected Operation";

      case http_status_heuristic_expiration:
        return &"Heuristic Expiration";

      case http_status_miscellaneous_warning:
        return &"Miscellaneous Warning";

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

      case http_status_multi_status:
        return &"Multi-Status";

      case http_status_already_reported:
        return &"Already Reported";

      case http_status_transformation_applied:
        return &"Transformation Applied";

      case http_status_im_used:
        return &"IM Used";

      case http_status_miscellaneous_persistent_warning:
        return &"Miscellaneous Persistent Warning";

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

      case http_status_switch_proxy:
        return &"Switch Proxy";

      case http_status_temporary_redirect:
        return &"Temporary Redirect";

      case http_status_permanent_redirect:
        return &"Permanent Redirect";

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

      case http_status_content_too_large:
        return &"Content Too Large";

      case http_status_uri_too_long:
        return &"URI Too Long";

      case http_status_unsupported_media_type:
        return &"Unsupported Media Type";

      case http_status_range_not_satisfiable:
        return &"Range Not Satisfiable";

      case http_status_expectation_failed:
        return &"Expectation Failed";

      case http_status_im_a_teapot:
        return &"I'm a Teapot";

      case http_status_page_expired:
        return &"Page Expired";

      case http_status_enhance_your_calm:
        return &"Enhance Your Calm";

      case http_status_misdirected_request:
        return &"Misdirected Request";

      case http_status_unprocessable_entity:
        return &"Unprocessable Entity";

      case http_status_locked:
        return &"Locked";

      case http_status_failed_dependency:
        return &"Failed Dependency";

      case http_status_too_early:
        return &"Too Early";

      case http_status_upgrade_required:
        return &"Upgrade Required";

      case http_status_precondition_required:
        return &"Precondition Required";

      case http_status_too_many_requests:
        return &"Too Many Requests";

      case http_status_request_header_fields_too_large_unofficial:
      case http_status_request_header_fields_too_large:
        return &"Request Header Fields Too Large";

      case http_status_login_timeout:
        return &"Login Timeout";

      case http_status_no_response:
        return &"No Response";

      case http_status_retry_with:
        return &"Retry With";

      case http_status_blocked_by_parental_control:
        return &"Blocked by Parental Controls";

      case http_status_unavailable_for_legal_reasons:
        return &"Unavailable for Legal Reasons";

      case http_status_client_closed_load_balanced_request:
        return &"Client Closed Load-Balanced Request";

      case http_status_invalid_x_forwarded_for:
        return &"Invalid X-Forwarded-For";

      case http_status_request_header_too_large:
        return &"Request Header Too Large";

      case http_status_ssl_certificate_error:
        return &"SSL Certificate Error";

      case http_status_ssl_certificate_required:
        return &"SSL Certificate Required";

      case http_status_http_request_sent_to_https_port:
        return &"HTTP Request Sent to HTTPS Port";

      case http_status_invalid_token:
        return &"Invalid Token";

      case http_status_client_closed_request:
        return &"Client Closed Request";

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

      case http_status_variant_also_negotiates:
        return &"Variant Also Negotiates";

      case http_status_insufficient_storage:
        return &"Insufficient Storage";

      case http_status_loop_detected:
        return &"Loop Detected";

      case http_status_bandwidth_limit_exceeded:
        return &"Bandwidth Limit Exceeded";

      case http_status_not_extended:
        return &"Not Extended";

      case http_status_network_authentication_required:
        return &"Network Authentication Required";

      case http_status_web_server_unknown_error:
        return &"Web Server Unknown Error";

      case http_status_web_server_is_down:
        return &"Web Server Is Down";

      case http_status_connection_timeout:
        return &"Connection Timeout";

      case http_status_origin_is_unreachable:
        return &"Origin Is Unreachable";

      case http_status_timeout_occured:
        return &"A Timeout Occurred";

      case http_status_ssl_handshake_failed:
        return &"SSL Handshake Failed";

      case http_status_invalid_ssl_certificate:
        return &"Invalid SSL Certificate";

      case http_status_railgun_error:
        return &"Railgun Error";

      case http_status_site_is_overloaded:
        return &"Site Is Overloaded";

      case http_status_site_is_frozen:
        return &"Site Is Frozen";

      case http_status_identity_provider_authentication_error:
        return &"Identity Provider Authentication Error";

      case http_status_network_read_timeout:
        return &"Network Read Timeout";

      case http_status_network_connect_timeout:
        return &"Network Connect Timeout";

      default:
        return &"Unknown Status";
      }
  }

HTTP_Response_Headers::
~HTTP_Response_Headers()
  {
  }

void
HTTP_Response_Headers::
encode(tinyfmt& fmt) const
  {
    // Write the status line. If `reason` is empty, a default reason string
    // is written. This function does not validate whether these fields
    // contain valid values.
    fmt << "HTTP/1.1 " << this->status << " ";
    if(this->reason.size() != 0)
      fmt << this->reason;
    else
      fmt << HTTP_Response_Headers::default_reason(this->status);

    // Write response headers. Empty headers are ignored.
    for(const auto& hr : this->headers)
      if(hr.first != "")
        fmt << "\r\n" << hr.first << ": " << hr.second.as_string();

    // Terminate the response with an empty line.
    fmt << "\r\n\r\n";
  }

}  // namespace poseidon
