// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_udp_client.hpp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_UDP_Client my_client;
extern Easy_Timer text_timer;

void
data_callback(shptrR<UDP_Socket> socket, Abstract_Fiber& /*fiber*/, Socket_Address&& addr, linear_buffer&& data)
  {
    cow_string str(data.data(), data.size());
    data.clear();
    POSEIDON_LOG_DEBUG(("example UDP client received data from `$1`: $2"), addr, str);
    (void) socket;
  }

void
timer_callback(shptrR<Abstract_Timer> /*timer*/, Abstract_Fiber& /*fiber*/, steady_time now)
  {
    Socket_Address addr("127.0.0.1:3801");
    cow_string str = format_string("ticks = $1", now.time_since_epoch());
    POSEIDON_LOG_INFO(("example UDP client sending data to `$1`: $2"), addr, str);
    my_client.udp_send(addr, str.data(), str.size());
  }

int
start_client()
  {
    my_client.open();
    text_timer.start((seconds) 2, (seconds) 1);
    POSEIDON_LOG_FATAL(("example UDP client started: local = $1"), my_client.local_address());
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_UDP_Client my_client(data_callback);
Easy_Timer text_timer(timer_callback);
int dummy = start_client();

}  // namespace
