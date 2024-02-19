// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "fwd.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/async_logger.hpp"
#include "static/timer_driver.hpp"
#include "static/async_task_executor.hpp"
#include "static/network_driver.hpp"
#include <locale.h>
#define UNW_LOCAL_ONLY  1
#include <libunwind.h>
#include <cxxabi.h>
#ifdef POSEIDON_ENABLE_MYSQL
#include "static/mysql_connector.hpp"
#endif
#ifdef POSEIDON_ENABLE_MONGO
#include "static/mongo_connector.hpp"
#endif
#ifdef POSEIDON_ENABLE_REDIS
#include "static/redis_connector.hpp"
#endif
namespace poseidon {

const ::locale_t c_locale = ::newlocale(0, "C", nullptr);
atomic_relaxed<int> exit_signal;
Main_Config& main_config = *new Main_Config;
Async_Logger& async_logger = *new Async_Logger;

Timer_Driver& timer_driver = *new Timer_Driver;
Async_Task_Executor& async_task_executor = *new Async_Task_Executor;
Network_Driver& network_driver = *new Network_Driver;
Fiber_Scheduler& fiber_scheduler = *new Fiber_Scheduler;

#ifdef POSEIDON_ENABLE_MYSQL
MySQL_Connector& mysql_connector = *new MySQL_Connector;
#endif
#ifdef POSEIDON_ENABLE_MONGO
Mongo_Connector& mongo_connector = *new Mongo_Connector;
#endif
#ifdef POSEIDON_ENABLE_REDIS
Redis_Connector& redis_connector = *new Redis_Connector;
#endif

bool
check_log_level(Log_Level level) noexcept
  {
    return async_logger.enabled(level);
  }

void
enqueue_log_message(const Log_Context& ctx, vfptr<cow_string&, void*> thunk, void* compose) noexcept
  try {
    cow_string sbuf;
    thunk(sbuf, compose);

    // Enqueue the message. If it is an error, wait until it is written.
    async_logger.enqueue(ctx, sbuf);
    if(ROCKET_UNEXPECT(ctx.level <= log_level_error))
      async_logger.synchronize();
  }
  catch(exception& stdex) {
    ::fprintf(stderr,
        "WARNING: Failed to write log message: %s\n"
        "[exception class `%s`]\n",
        stdex.what(), typeid(stdex).name());
  }

void
backtrace_and_throw(const Log_Context& ctx, vfptr<cow_string&, void*> thunk, void* compose)
  {
    cow_string sbuf;
    thunk(sbuf, compose);

    // Remove trailing space characters from the message.
    while(!sbuf.empty() && is_any_of(sbuf.back(), {' ','\t','\r','\n'}))
      sbuf.pop_back();

    // Append the source location and function name.
    ::rocket::ascii_numput nump;
    nump.put_DU(ctx.line);
    sbuf += "\n[thrown from '";
    sbuf += ctx.file;
    sbuf += ":";
    sbuf.append(nump.data(), nump.size());
    sbuf += "' inside function `";
    sbuf += ctx.func;
    sbuf += "`]";

    // Calculate the number of stack frames.
    ::unw_context_t unw_ctx[1];
    ::unw_cursor_t unw_cur[1];
    size_t nframes = 0;

    if((::unw_getcontext(unw_ctx) == 0) && (::unw_init_local(unw_cur, unw_ctx) == 0))
      while(::unw_step(unw_cur) > 0)
        ++ nframes;

    if(nframes != 0) {
      // Determine the width of the frame index field.
      static_vector<char, 8> numfield;
      nump.put_DU(nframes);
      numfield.assign(nump.size(), ' ');

      // Append frames to the exception message.
      sbuf += "\n[stack backtrace:";
      nframes = 0;
      ::unw_init_local(unw_cur, unw_ctx);

      char* fn_ptr = nullptr;
      size_t fn_size = 0;
      const auto fn_guard = ::rocket::make_unique_handle(&fn_ptr,
                                [](char** p) { if(*p) ::free(*p);  });

      char unw_name[1024];
      ::unw_word_t unw_offset;

      while(::unw_step(unw_cur) > 0) {
        // * frame index
        nump.put_DU(++ nframes);
        ::std::reverse_copy(nump.begin(), nump.end(), numfield.mut_rbegin());
        sbuf += "\n  ";
        sbuf.append(numfield.data(), numfield.size());
        sbuf += ") ";

        // * instruction pointer
        ::unw_get_reg(unw_cur, UNW_REG_IP, &unw_offset);
        nump.put_XU(unw_offset);
        sbuf.append(nump.data(), nump.size());

        if(::unw_get_proc_name(unw_cur, unw_name, sizeof(unw_name), &unw_offset) == 0) {
          // Demangle the function name. If `__cxa_demangle()` returns a non-null
          // pointer, `fn_ptr` will have been reallocated to `fn` and it will
          // point to the demangled name.
          char* fn = ::abi::__cxa_demangle(unw_name, fn_ptr, &fn_size, nullptr);
          if(fn)
            fn_ptr = fn;
          else
            fn = unw_name;

          // * function name and offset
          sbuf += " `";
          sbuf += fn;
          sbuf += "` +";
          nump.put_XU(unw_offset);
          sbuf.append(nump.data(), nump.size());
        }
        else
          sbuf += " (unknown function)";
      }

      sbuf += "\n  -- end of stack backtrace]";
    }

    // Throw a standard exception. It keeps a mysterious reference-counting
    // string but how can we make use of it to avoid the copy?
    throw ::std::runtime_error(sbuf.c_str());
  }

}  // namespace poseidon
