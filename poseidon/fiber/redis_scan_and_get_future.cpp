// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_scan_and_get_future.hpp"
#include "../redis/redis_connection.hpp"
#include "../static/redis_connector.hpp"
#include "../utils.hpp"
namespace poseidon {

Redis_Scan_and_Get_Future::
Redis_Scan_and_Get_Future(Redis_Connector& connector, uniptr<Redis_Connection>&& conn_opt,
                          const cow_string& pattern)
  {
    this->m_ctr = &connector;
    this->m_conn = move(conn_opt);
    this->m_pattern = pattern;
  }

Redis_Scan_and_Get_Future::
Redis_Scan_and_Get_Future(Redis_Connector& connector, const cow_string& pattern)
  {
    this->m_ctr = &connector;
    this->m_pattern = pattern;
  }

Redis_Scan_and_Get_Future::
~Redis_Scan_and_Get_Future()
  {
  }

void
Redis_Scan_and_Get_Future::
do_on_abstract_future_initialize()
  {
    if(!this->m_conn)
      this->m_conn = this->m_ctr->allocate_default_connection();

    // Scan for keys until the end cursor has been reached. Each reply shall be
    // an array of two elements. The first element is the next cursor. The second
    // element is an array of matching keys.
    cow_vector<cow_string> cmd;
    cow_string status;
    Redis_Value value;

    cmd.resize(4);
    cmd.mut(0) = &"SCAN";
    cmd.mut(1) = &"0";  // cursor
    cmd.mut(2) = &"MATCH";
    cmd.mut(3) = this->m_pattern;

  do_scan_more_:
    this->m_conn->execute(cmd);
    this->m_conn->fetch_reply(status, value);

    auto replies = move(value.mut_array().mut(1).mut_array());
    auto reply_it = replies.mut_begin();
    while(reply_it != replies.end()) {
      POSEIDON_LOG_TRACE((" SCAN => $1"), *reply_it);
      this->m_res.try_emplace(move(reply_it->mut_string()));
      ++ reply_it;
    }

    cmd.mut(1) = move(value.mut_array().mut(0).mut_string());
    if(cmd[1] != "0")
      goto do_scan_more_;

    if(this->m_res.empty())
      return;

    // Append all unique keys in ascending order. There is a one-to-one mapping
    // from the arguments to the elements in the reply array, so the order of
    // keys must be preserved.
    cmd.resize(1);
    cmd.mut(0) = &"MGET";

    cmd.reserve(cmd.size() + this->m_res.size());
    auto res_it = this->m_res.mut_begin();
    while(res_it != this->m_res.end()) {
      cmd.emplace_back(res_it->first);
      ++ res_it;
    }

    // Get values. The reply shall be an array.
    this->m_conn->execute(cmd);
    this->m_conn->fetch_reply(status, value);

    res_it = this->m_res.mut_begin();
    replies = move(value.mut_array());
    reply_it = replies.mut_begin();
    while((res_it != this->m_res.end()) && (reply_it != replies.end())) {
      if(!reply_it->is_string())
        res_it = this->m_res.erase(res_it);
      else {
        POSEIDON_LOG_TRACE((" MGET => $1 ($2)"), res_it->first, reply_it->as_string_length());
        res_it->second = move(reply_it->mut_string());
        ++ res_it;
      }
      ++ reply_it;
    }
  }

void
Redis_Scan_and_Get_Future::
do_on_abstract_future_finalize()
  {
    if(!this->m_conn)
      return;

    if(this->m_conn->reset())
      this->m_ctr->pool_connection(move(this->m_conn));
  }

void
Redis_Scan_and_Get_Future::
do_on_abstract_task_execute()
  {
    this->do_abstract_future_initialize_once();
  }

}  // namespace poseidon
