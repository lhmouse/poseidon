// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

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

    cow_string m_service_uri;
    cow_string m_db;
    bool m_reply_available;
    bool m_reset_clear;
    char m_reserved_1;
    char m_reserved_2;
    steady_time m_time_pooled;

    uniptr_mongoc_client m_mongo;
    scoped_bson m_reply;
    uniptr_mongoc_cursor m_cursor;

  public:
    // Sets connection parameters. This function does not attempt to connect
    // to the server, and is not blocking. If you need to set a password in
    // plaintext, set `password_mask` to zero.
    Mongo_Connection(cow_stringR service_uri, cow_stringR password, uint32_t password_mask);

  public:
    Mongo_Connection(const Mongo_Connection&) = delete;
    Mongo_Connection& operator=(const Mongo_Connection&) & = delete;
    ~Mongo_Connection();

    // Gets the URI from the constructor.
    cow_stringR
    service_uri() const noexcept
      { return this->m_service_uri;  }

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
