// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MONGO_MONGO_CONNECTION_
#define POSEIDON_MONGO_MONGO_CONNECTION_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../mongo/mongo_value.hpp"
#include "../third/mongo_fwd.hpp"
namespace poseidon {

class Mongo_Connection
  {
  private:
    friend class Mongo_Connector;

    cow_string m_server;
    cow_string m_db;
    cow_string m_user;
    uint16_t m_port;

    bool m_reset_clear;
    bool m_reply_available;
    uniptr_mongoc_client m_mongo;
    scoped_bson m_reply;
    uniptr_mongoc_cursor m_cursor;

  public:
    // Sets connection parameters. This function does not attempt to connect
    // to the server, and is not blocking.
    Mongo_Connection(cow_stringR server, uint16_t port, cow_stringR db, cow_stringR user, cow_stringR passwd);

  public:
    Mongo_Connection(const Mongo_Connection&) = delete;
    Mongo_Connection& operator=(const Mongo_Connection&) & = delete;
    ~Mongo_Connection();

    // Get connection parameters.
    cow_stringR
    server() const noexcept
      { return this->m_server;  }

    uint16_t
    port() const noexcept
      { return this->m_port;  }

    cow_stringR
    db() const noexcept
      { return this->m_db;  }

    cow_stringR
    user() const noexcept
      { return this->m_user;  }

    // Resets the connection so it can be reused by another thread. This is a
    // blocking functions. DO NOT ATTEMPT TO REUSE THE CONNECTION IF THIS
    // FUNCTION RETURNS `false`.
    // Returns whether the connection may be safely reused.
    bool
    reset() noexcept;

    // Executes a command in BSON format with no option on the default database. It
    // is recommended that you pass a `scoped_bson` object.
    void
    execute_bson(const ::bson_t* bson_cmd);

    // Fetches a document from the reply to the last command. This function must be
    // be called after `execute_bson()`. If the command has produced a reply which
    // does not contain a cursor, a pointer to the reply document is returned. If
    // the command has produced a reply which contains a cursor, a cursor is created
    // where subsequent documents will be fetched. In  either case, the caller must
    // not free the result `bson_t` object. If there is no more document to fetch, a
    // null pointer is returned.
    const ::bson_t*
    fetch_reply_bson_opt();

    // Executes a command in BSON format with no option on the default database.
    // Reference: https://www.mongodb.com/docs/manual/reference/command/nav-crud/
    void
    execute(const Mongo_Document& cmd);

    // Fetches a document from the reply to the last command. This function must be
    // called after `execute()`. `output` is cleared before fetching any data. If
    // the command has produced a reply which does not contain a cursor, the reply
    // is converted to a `Mongo_Document` literally and stored into `output`. If the
    // command has produced a reply which contains a cursor, a cursor is created
    // where subsequent documents will be fetched. If there is no more document to
    // fetch, `false` is returned.
    bool
    fetch_reply(Mongo_Document& output);
  };

}  // namespace poseidon
#endif
