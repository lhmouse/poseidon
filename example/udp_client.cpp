// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy_udp_client.hpp"
#include "../poseidon/easy_timer.hpp"
#include "../poseidon/static/async_logger.hpp"
#include "../poseidon/utils.hpp"

namespace {
using namespace ::poseidon;

extern Easy_UDP_Client my_client;
extern Easy_Timer text_timer;

void
data_callback(Socket_Address&& addr, linear_buffer&& data)
  {
    cow_string str(data.data(), data.size());
    data.clear();

    POSEIDON_LOG_DEBUG(("example UDP client received data from `$1`: $2"), addr, str);
  }

void
timer_callback(int64_t now)
  {
    static Socket_Address addr((::in6_addr) IN6ADDR_LOOPBACK_INIT, 3801);
    static uint32_t index;
    cow_string str;
    format(str, "packet $1", ++index);
    (void) now;

    POSEIDON_LOG_INFO(("example UDP client sending data to `$1`: $2"), addr, str);
    my_client.send(addr, str);
  }

int
start_client()
  {
    my_client.start();
    text_timer.start(0, 1000'000'000);
    POSEIDON_LOG_FATAL(("example UDP client started: local = $1"), my_client.local_address());
    return 0;
  }

// Start the client when this shared library is being loaded.
Easy_UDP_Client my_client(data_callback);
Easy_Timer text_timer(timer_callback);
int dummy = start_client();

}  // namespace
