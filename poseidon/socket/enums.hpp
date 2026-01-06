// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_ENUMS_
#define POSEIDON_SOCKET_ENUMS_

#include "../fwd.hpp"
namespace poseidon {

enum IP_Address_Class : uint8_t
  {
    ip_address_unspecified  = 0,  // all zeroes
    ip_address_reserved     = 1,
    ip_address_public       = 2,
    ip_address_loopback     = 3,
    ip_address_private      = 4,
    ip_address_link_local   = 5,
    ip_address_multicast    = 6,
    ip_address_broadcast    = 7,  // IPv4 only
  };

enum Socket_State : uint8_t
  {
    socket_pending      = 0,  // TCP, SSL
    socket_established  = 1,  // TCP, SSL, UDP
    socket_closing      = 2,  // TCP, SSL
    socket_closed       = 3,  // TCP, SSL, UDP
  };

enum HTTP_Payload_Type : uint8_t
  {
    http_payload_normal   = 0,
    http_payload_empty    = 1,
    http_payload_connect  = 2,
  };

}  // namespace poseidon
#endif
