// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_query_future.hpp"
#include "../mongo/mongo_connection.hpp"
#include "../static/mongo_connector.hpp"
#include "../utils.hpp"
namespace poseidon {

Mongo_Query_Future::
Mongo_Query_Future(Mongo_Connector& connector, uniptr<Mongo_Connection>&& conn_opt,
                   const Mongo_Document& cmd)
  {
    this->m_ctr = &connector;
    this->m_conn = move(conn_opt);
    this->m_res.cmd = cmd;
  }

Mongo_Query_Future::
Mongo_Query_Future(Mongo_Connector& connector, const Mongo_Document& cmd)
  {
    this->m_ctr = &connector;
    this->m_res.cmd = cmd;
  }

Mongo_Query_Future::
~Mongo_Query_Future()
  {
  }

void
Mongo_Query_Future::
do_on_abstract_future_execute()
  {
    if(!this->m_conn)
      this->m_conn = this->m_ctr->allocate_default_connection();

    this->m_conn->execute(this->m_res.cmd);

    Mongo_Document doc;
    while(this->m_conn->fetch_reply(doc))
      this->m_res.reply_docs.push_back(move(doc));
  }

void
Mongo_Query_Future::
do_on_abstract_future_finalize()
  {
    if(this->m_conn && this->m_conn->reset())
      this->m_ctr->pool_connection(move(this->m_conn));
  }

}  // namespace poseidon
