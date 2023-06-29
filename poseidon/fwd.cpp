// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "precompiled.ipp"
#include "fwd.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/async_logger.hpp"
#include "static/timer_driver.hpp"
#include "static/async_task_executor.hpp"
#include "static/network_driver.hpp"
namespace poseidon {

atomic_relaxed<int> exit_signal;
Main_Config& main_config = *new Main_Config;
Fiber_Scheduler& fiber_scheduler = *new Fiber_Scheduler;

Async_Logger& async_logger = *new Async_Logger;
Timer_Driver& timer_driver = *new Timer_Driver;
Async_Task_Executor& async_task_executor = *new Async_Task_Executor;
Network_Driver& network_driver = *new Network_Driver;

bool
do_async_logger_check_level(Log_Level level) noexcept
  {
    return async_logger.enabled(level);
  }

void
do_async_logger_enqueue(const Log_Context& ctx, vfptr<cow_string&, const void*> invoke, const void* compose) noexcept
  try {
    cow_string sbuf;
    invoke(sbuf, compose);
    async_logger.enqueue(ctx, sbuf);

    if(ctx.level <= log_level_error)
      async_logger.synchronize();
  }
  catch(::std::exception& stdex) {
    ::fprintf(stderr,
        "WARNING: Could not compose log message: %s\n",
        stdex.what());
  }

}  // namespace poseidon
