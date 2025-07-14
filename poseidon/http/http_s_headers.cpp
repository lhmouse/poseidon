// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "http_s_headers.hpp"
#include "../utils.hpp"
#include <http_parser.h>
namespace poseidon {

HTTP_S_Headers::
~HTTP_S_Headers()
  {
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
    if(this->reason != "")
      fmt << this->reason;
    else
      fmt << ::http_status_str(static_cast<::http_status>(this->status));

    // Write response headers. Empty headers are ignored.
    for(const auto& hr : this->headers)
      if(hr.first != "")
        fmt << "\r\n" << hr.first << ": " << hr.second.as_string();

    // Terminate the response with an empty line.
    fmt << "\r\n\r\n";
  }

}  // namespace poseidon
