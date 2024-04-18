// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../poseidon/xprecompiled.hpp"
#include "../poseidon/easy/easy_udp_server.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

static Easy_UDP_Server my_server(
  // callback
  *[](shptrR<UDP_Socket> socket, Abstract_Fiber& fiber, IPv6_Address&& addr,
      linear_buffer&& data)
  {
    (void) fiber;

    POSEIDON_LOG_FATAL(("example UDP server received data from `$1`: $2"), addr, data);
    socket->udp_send(addr, data);
    data.clear();
  });

void
poseidon_module_main()
  {
    static constexpr char bind_addr[] = "[::]:3801";
    my_server.start(&bind_addr);
    POSEIDON_LOG_FATAL(("example UDP server started: $1"), bind_addr);
  }
