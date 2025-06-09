// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "fwd.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/logger.hpp"
#include "static/timer_driver.hpp"
#include "static/task_executor.hpp"
#include "static/network_driver.hpp"
#include <locale.h>
#include "static/mysql_connector.hpp"
#include "static/mongo_connector.hpp"
#include "static/redis_connector.hpp"
namespace poseidon {

const ::locale_t c_locale = ::newlocale(0, "C", nullptr);
const cow_string empty_cow_string;
const phcow_string empty_phcow_string;

atomic_relaxed<int> exit_signal;
Main_Config& main_config = *new Main_Config;
Logger& logger = *new Logger;

Timer_Driver& timer_driver = *new Timer_Driver;
Task_Executor& task_executor = *new Task_Executor;
Network_Driver& network_driver = *new Network_Driver;
Fiber_Scheduler& fiber_scheduler = *new Fiber_Scheduler;

MySQL_Connector& mysql_connector = *new MySQL_Connector;
Mongo_Connector& mongo_connector = *new Mongo_Connector;
Redis_Connector& redis_connector = *new Redis_Connector;

}  // namespace poseidon
