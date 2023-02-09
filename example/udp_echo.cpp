// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy_udp_server.hpp"
#include "../poseidon/static/async_logger.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_UDP_Server my_server;

void
data_callback(Socket_Address&& addr, linear_buffer&& data)
  {
    cow_string str(data.data(), data.size());
    data.clear();
    POSEIDON_LOG_WARN(("example UDP server received data from `$1`: $2"), addr, str);
    my_server.udp_send(addr, str.data(), str.size());
  }

int
start_server()
  {
    Socket_Address addr(sref("[::]:3801"));
    my_server.start(addr);
    POSEIDON_LOG_ERROR(("example UDP server started: bind = $1"), my_server.local_address());
    return 0;
  }

// Start the server when this shared library is being loaded.
Easy_UDP_Server my_server(data_callback);
int dummy = start_server();

}  // namespace
