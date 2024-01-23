// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../poseidon/precompiled.ipp"
#include "../poseidon/easy/easy_timer.hpp"
#include "../poseidon/utils.hpp"
namespace {
using namespace ::poseidon;

extern Easy_Timer my_timer;

void
timer_callback(shR<Abstract_Timer> /*timer*/, Abstract_Fiber& /*fiber*/,
               steady_time now)
  {
    POSEIDON_LOG_WARN(("example timer: now = $1"), now.time_since_epoch());
  }

int
start_timer()
  {
    my_timer.start(5s, 1s);
    POSEIDON_LOG_ERROR(("example timer started"));
    return 0;
  }

// Start the timer when this shared library is being loaded.
Easy_Timer my_timer(timer_callback);
int dummy = start_timer();

}  // namespace
