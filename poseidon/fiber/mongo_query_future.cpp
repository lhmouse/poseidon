// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_query_future.hpp"
#include "../mongo/mongo_connection.hpp"
#include "../static/mongo_connector.hpp"
#include "../utils.hpp"
namespace poseidon {

Mongo_Query_Future::
Mongo_Query_Future(Mongo_Connector& connector, Mongo_Document cmd)
  {
    this->m_connector = &connector;
    this->m_res.cmd = move(cmd);
  }

Mongo_Query_Future::
~Mongo_Query_Future()
  {
  }

void
Mongo_Query_Future::
do_on_abstract_future_execute()
  {
    // Get a connection. Before this function returns, the connection should
    // be reset and put back.
    uniptr<Mongo_Connection> conn = this->m_connector->allocate_default_connection();
    const auto conn_guard = make_unique_handle(&conn,
        [=](uniptr<Mongo_Connection>* p) {
          if((*p)->reset())
            this->m_connector->pool_connection(move(*p));
        });

    // Execute the command.
    conn->execute(this->m_res.cmd);

    // Fetch result documents.
    Mongo_Document doc;
    while(conn->fetch_reply(doc))
      this->m_res.reply_docs.push_back(move(doc));

    POSEIDON_LOG_DEBUG((
        "Finished BSON command: $1",
        "$2 document(s) returned"),
        this->m_res.cmd,
        this->m_res.reply_docs.size());
  }

void
Mongo_Query_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
