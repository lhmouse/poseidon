// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "fwd.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/logger.hpp"
#include "static/timer_scheduler.hpp"
#include "static/task_scheduler.hpp"
#include "static/network_scheduler.hpp"
#include "static/mysql_connector.hpp"
#include "static/mongo_connector.hpp"
#include "static/redis_connector.hpp"
#include <asteria/utils.hpp>
#include <locale.h>
namespace poseidon {
namespace {

::locale_t
do_create_c_locale()
  {
    ::locale_t loc = ::newlocale(0, "C.UTF-8", nullptr);
    if(!loc)
      loc = ::newlocale(0, "C", nullptr);
    if(!loc)
      ASTERIA_TERMINATE(("Could not initialize C locale: ${errno:full}"));
    return loc;
  }

cow_string
do_get_hostname()
  {
    char str[HOST_NAME_MAX];
    if(::gethostname(str, sizeof(str)) != 0)
      ASTERIA_TERMINATE(("Could not read hostname: ${errno:full}"));
    return cow_string(str, ::strlen(str));
  }

}  // namespace

const ::locale_t c_locale = do_create_c_locale();
const cow_string hostname = do_get_hostname();
const cow_string empty_cow_string;
const phcow_string empty_phcow_string;

Main_Config main_config;
Logger logger;
Timer_Scheduler timer_scheduler;
Task_Scheduler task_scheduler;
Network_Scheduler network_scheduler;
Fiber_Scheduler fiber_scheduler;
MySQL_Connector mysql_connector;
Mongo_Connector mongo_connector;
Redis_Connector redis_connector;

}  // namespace poseidon
