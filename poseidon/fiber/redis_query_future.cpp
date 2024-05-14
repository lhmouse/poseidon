// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_query_future.hpp"
#include "../redis/redis_connection.hpp"
#include "../static/redis_connector.hpp"
#include "../utils.hpp"
namespace poseidon {

Redis_Query_Future::
Redis_Query_Future(Redis_Connector& connector, const cow_vector<cow_string>& cmd)
  {
    this->m_ctr = &connector;
    this->m_res.cmd = cmd;
  }

Redis_Query_Future::
~Redis_Query_Future()
  {
  }

void
Redis_Query_Future::
do_on_abstract_future_execute()
  {
    this->m_conn = this->m_ctr->allocate_default_connection();
    this->m_conn->execute(this->m_res.cmd.data(), this->m_res.cmd.size());

    // We don't do pipelining, so it's assumed that there is exactly one reply
    // for this command.
    this->m_conn->fetch_reply(this->m_res.reply);
  }

void
Redis_Query_Future::
do_on_abstract_future_finalize() noexcept
  {
    if(this->m_conn && this->m_conn->reset())
      this->m_ctr->pool_connection(move(this->m_conn));
  }

}  // namespace poseidon
