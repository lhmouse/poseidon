// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_ENUMS_
#define POSEIDON_HTTP_ENUMS_

#include "../fwd.hpp"
namespace poseidon {

enum HTTP_Method : uint64_t
  {
    http_NULL      = 0,
    http_OPTIONS   = 0x0'00'53'4E'4F'49'54'50'4F,
    http_GET       = 0x0'00'00'00'00'00'54'45'47,
    http_HEAD      = 0x0'00'00'00'00'44'41'45'48,
    http_POST      = 0x0'00'00'00'00'54'53'4F'50,
    http_PUT       = 0x0'00'00'00'00'00'54'55'50,
    http_DELETE    = 0x0'00'00'45'54'45'4C'45'44,
    http_TRACE     = 0x0'00'00'00'45'43'41'52'54,
    http_CONNECT   = 0x0'00'54'43'45'4E'4E'4F'43,
    http_PATCH     = 0x0'00'00'00'48'43'54'41'50,
  };

enum HTTP_Status : uint16_t
  {
    http_status_null                             =   0,
    http_status_continue                         = 100,
    http_status_switching_protocols              = 101,
    http_status_ok                               = 200,
    http_status_connection_established           = 200,
    http_status_created                          = 201,
    http_status_accepted                         = 202,
    http_status_nonauthoritative_information     = 203,
    http_status_no_content                       = 204,
    http_status_reset_content                    = 205,
    http_status_partial_content                  = 206,
    http_status_multiple_choices                 = 300,
    http_status_moved_permanently                = 301,
    http_status_found                            = 302,
    http_status_see_other                        = 303,
    http_status_not_modified                     = 304,
    http_status_use_proxy                        = 305,
    http_status_temporary_redirect               = 307,
    http_status_bad_request                      = 400,
    http_status_unauthorized                     = 401,
    http_status_payment_required                 = 402,
    http_status_forbidden                        = 403,
    http_status_not_found                        = 404,
    http_status_method_not_allowed               = 405,
    http_status_not_acceptable                   = 406,
    http_status_proxy_authentication_required    = 407,
    http_status_request_timeout                  = 408,
    http_status_conflict                         = 409,
    http_status_gone                             = 410,
    http_status_length_required                  = 411,
    http_status_precondition_failed              = 412,
    http_status_payload_too_large                = 413,
    http_status_uri_too_long                     = 414,
    http_status_unsupported_media_type           = 415,
    http_status_range_not_satisfiable            = 416,
    http_status_expectation_failed               = 417,
    http_status_upgrade_required                 = 426,
    http_status_internal_server_error            = 500,
    http_status_not_implemented                  = 501,
    http_status_bad_gateway                      = 502,
    http_status_service_unavailable              = 503,
    http_status_gateway_timeout                  = 504,
    http_status_http_version_not_supported       = 505,
  };

enum WS_Opcode : uint8_t
  {
    ws_CONTINUATION  =  0,
    ws_TEXT          =  1,
    ws_BINARY        =  2,
    ws_CLOSE         =  8,
    ws_PING          =  9,
    ws_PONG          = 10,
  };

enum WS_Status : uint16_t
  {
    ws_status_null                  =    0,
    ws_status_normal                = 1000,
    ws_status_going_away            = 1001,
    ws_status_protocol_error        = 1002,
    ws_status_not_acceptable        = 1003,
    ws_status_no_status_code        = 1005,
    ws_status_no_close_frame        = 1006,
    ws_status_message_data_error    = 1007,
    ws_status_policy_violation      = 1008,
    ws_status_message_too_large     = 1009,
    ws_status_extension_required    = 1010,
    ws_status_unexpected_error      = 1011,
    ws_status_service_restart       = 1012,
    ws_status_try_again_later       = 1013,
    ws_status_bad_gateway           = 1014,
    ws_status_tls_error             = 1015,
    ws_status_unauthorized          = 3000,
    ws_status_forbidden             = 3003,
    ws_status_timeout               = 3008,
  };

}  // namespace poseidon
#endif
