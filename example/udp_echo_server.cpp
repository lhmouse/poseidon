// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.hpp"
#include "../poseidon/easy/easy_udp_server.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_UDP_Server my_server;

void
data_callback(shptrR<UDP_Socket> socket, Abstract_Fiber& /*fiber*/, Socket_Address&& addr, linear_buffer&& data)
  {
    POSEIDON_LOG_FATAL(("example UDP server received data from `$1`: $2"), addr, data);
    socket->udp_send(addr, data);
    data.clear();
  }

int
start_server()
  {
    my_server.start(sref("[::]:3801"));
    POSEIDON_LOG_FATAL(("example UDP server started: bind = $1"), my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_UDP_Server my_server(data_callback);
int dummy = start_server();

}  // namespace
