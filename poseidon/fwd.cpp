// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "fwd.hpp"
#include "static/main_config.hpp"
#include "static/fiber_scheduler.hpp"
#include "static/logger.hpp"
#include "static/timer_driver.hpp"
#include "static/task_executor.hpp"
#include "static/network_driver.hpp"
#include <locale.h>
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
Logger& logger = *new Logger;

Timer_Driver& timer_driver = *new Timer_Driver;
Task_Executor& task_executor = *new Task_Executor;
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

}  // namespace poseidon
