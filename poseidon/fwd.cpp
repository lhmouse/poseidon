// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "fwd.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/logger.hpp"
#include "static/timer_scheduler.hpp"
#include "static/task_scheduler.hpp"
#include "static/network_scheduler.hpp"
#include <locale.h>
#include "static/mysql_connector.hpp"
#include "static/mongo_connector.hpp"
#include "static/redis_connector.hpp"
namespace poseidon {
namespace {

::locale_t
do_create_c_locale()
  {
    ::locale_t loc = ::newlocale(0, "C.UTF-8", nullptr);
    if(!loc)
      loc = ::newlocale(0, "C", nullptr);
    if(!loc)
      ::std::terminate();
    return loc;
  }

cow_string
do_get_hostname()
  {
    char str[HOST_NAME_MAX];
    if(::gethostname(str, sizeof(str)) != 0)
      ::std::terminate();
    return cow_string(str, ::strlen(str));
  }

}  // namespace

const ::locale_t c_locale = do_create_c_locale();
const cow_string hostname = do_get_hostname();
const cow_string empty_cow_string;
const phcow_string empty_phcow_string;

atomic_relaxed<int> exit_signal;
Main_Config& main_config = *new Main_Config;
Logger& logger = *new Logger;

Timer_Scheduler& timer_scheduler = *new Timer_Scheduler;
Task_Scheduler& task_scheduler = *new Task_Scheduler;
Network_Scheduler& network_scheduler = *new Network_Scheduler;
Fiber_Scheduler& fiber_scheduler = *new Fiber_Scheduler;

MySQL_Connector& mysql_connector = *new MySQL_Connector;
Mongo_Connector& mongo_connector = *new Mongo_Connector;
Redis_Connector& redis_connector = *new Redis_Connector;

}  // namespace poseidon
