// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_query_future.hpp"
#include "../redis/redis_connection.hpp"
#include "../static/redis_connector.hpp"
#include "../utils.hpp"
namespace poseidon {

Redis_Query_Future::
Redis_Query_Future(Redis_Connector& connector, vector<cow_string> cmd)
  {
    this->m_ctr = &connector;
    this->m_res.cmd = move(cmd);
  }

Redis_Query_Future::
~Redis_Query_Future()
  {
  }

void
Redis_Query_Future::
do_on_abstract_future_execute()
  {
    // Get a connection. Before this function returns, the connection should
    // be reset and put back.
    uniptr<Redis_Connection> conn = this->m_ctr->allocate_default_connection();
    const auto conn_guard = make_unique_handle(&conn,
        [&](...) {
          if(conn && conn->reset())
            this->m_ctr->pool_connection(move(conn));
        });

    // Execute the command.
    conn->execute(this->m_res.cmd.data(), this->m_res.cmd.size());

    // Fetch its reply. We don't do pipelining, so it's assumed that there is
    // exactly one reply for this command.
    bool fetched = conn->fetch_reply(this->m_res.reply);

    POSEIDON_LOG_DEBUG((
        "Finished redis command: $1",
        "$2 reply --> $3"),
        implode(this->m_res.cmd, ' '),
        fetched ? "failed to receive" : "received", this->m_res.reply);
  }

void
Redis_Query_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
