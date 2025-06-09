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
    this->m_res.pattern = pattern;
  }

Redis_Scan_and_Get_Future::
Redis_Scan_and_Get_Future(Redis_Connector& connector, const cow_string& pattern)
  {
    this->m_ctr = &connector;
    this->m_res.pattern = pattern;
  }

Redis_Scan_and_Get_Future::
~Redis_Scan_and_Get_Future()
  {
  }

void
Redis_Scan_and_Get_Future::
do_on_abstract_future_execute()
  {
    if(!this->m_conn)
      this->m_conn = this->m_ctr->allocate_default_connection();

    // Scan for keys until the end cursor has been reached. Each reply shall be
    // an array of two elements. The first element is the next cursor. The second
    // element is an array of matching keys.
    cow_vector<cow_string> cmd;
    cmd.resize(4);
    cmd.mut(0) = &"SCAN";
    cmd.mut(1) = &"0";  // cursor
    cmd.mut(2) = &"MATCH";
    cmd.mut(3) = this->m_res.pattern;

    this->m_conn->execute(cmd);
    Redis_Value reply;
    do {
      this->m_conn->fetch_reply(reply);
      cmd.mut(1) = move(reply.mut_array().mut(0).mut_string());
      auto keys = move(reply.mut_array().mut(1).mut_array());

      auto key_it = keys.mut_begin();
      while(key_it != keys.end()) {
        POSEIDON_LOG_TRACE((" SCAN => $1"), *key_it);
        this->m_res.pairs.try_emplace(move(key_it->mut_string()));
        ++ key_it;
      }
    } while(cmd[1] != "0");

    if(this->m_res.pairs.empty())
      return;

    // Append all unique keys in ascending order. There is a one-to-one mapping
    // from the arguments to the elements in the reply array, so the order of
    // keys must be preserved.
    cmd.resize(1);
    cmd.mut(0) = &"MGET";

    cmd.reserve(cmd.size() + this->m_res.pairs.size());
    auto res_it = this->m_res.pairs.mut_begin();
    while(res_it != this->m_res.pairs.end()) {
      cmd.emplace_back(res_it->first);
      ++ res_it;
    }

    // Get them. The reply shall be an array.
    this->m_conn->execute(cmd);
    this->m_conn->fetch_reply(reply);
    auto values = move(reply.mut_array());

    res_it = this->m_res.pairs.mut_begin();
    auto val_it = values.mut_begin();
    while((res_it != this->m_res.pairs.end()) && (val_it != values.end())) {
      if(!val_it->is_string())
        res_it = this->m_res.pairs.erase(res_it);
      else {
        POSEIDON_LOG_TRACE((" MGET <= $1 ($2)"), res_it->first, val_it->as_string_length());
        res_it->second = move(val_it->mut_string());
        ++ res_it;
      }
      ++ val_it;
    }
  }

void
Redis_Scan_and_Get_Future::
do_on_abstract_future_finalize()
  {
    if(this->m_conn && this->m_conn->reset())
      this->m_ctr->pool_connection(move(this->m_conn));
  }

}  // namespace poseidon
