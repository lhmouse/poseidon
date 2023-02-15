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
data_callback(shared_ptrR<UDP_Socket> socket, Socket_Address&& addr, linear_buffer&& data)
  {
    cow_string str(data.data(), data.size());
    data.clear();
    POSEIDON_LOG_DEBUG(("example UDP client received data from `$1`: $2"), addr, str);
    (void) socket;
  }

void
timer_callback(milliseconds now)
  {
    Socket_Address addr(sref("127.0.0.1:3801"));
    static uint32_t index;
    cow_string str = format_string("packet $1", ++index);
    POSEIDON_LOG_INFO(("example UDP client sending data to `$1`: $2"), addr, str);
    my_client.udp_send(addr, str.data(), str.size());
    (void) now;
  }

int
start_client()
  {
    my_client.start();
    text_timer.start(seconds(2), seconds(1));
    POSEIDON_LOG_FATAL(("example UDP client started: local = $1"), my_client.local_address());
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_UDP_Client my_client(data_callback);
Easy_Timer text_timer(timer_callback);
int dummy = start_client();

}  // namespace
