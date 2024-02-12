// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_connection.hpp"
//#include "mongo_value.hpp"
#include "../utils.hpp"
namespace poseidon {

Mongo_Connection::
Mongo_Connection(cow_stringR server, uint16_t port, cow_stringR db, cow_stringR user, cow_stringR passwd)
  {
    this->m_server = server;
    this->m_port = port;
    this->m_db = db;
    this->m_user = user;

    // Parse the server name.
    uniptr_mongoc_uri uri(::mongoc_uri_new_for_host_port(server.safe_c_str(), port));
    if(!uri)
      POSEIDON_THROW(("Invalid Mongo server name `$1:$2`"), server, port);

    // Fill connection parameters.
    if(!::mongoc_uri_set_database(uri, db.safe_c_str()))
      POSEIDON_THROW(("Invalid Mongo database name `$1`"), db);

    if(!::mongoc_uri_set_username(uri, user.safe_c_str()))
      POSEIDON_THROW(("Invalid Mongo user name `$1`"), user);

    if(!::mongoc_uri_set_password(uri, passwd.safe_c_str()))
      POSEIDON_THROW(("Invalid Mongo password"));

    // Create the client object. This does not initiate the connection.
    ::bson_error_t error;
    if(!this->m_mongo.reset(::mongoc_client_new_from_uri_with_error(uri, &error)))
      POSEIDON_THROW((
          "Could not create Mongo client object: ERROR $1.$2: $3",
          "[`mongoc_client_new_from_uri_with_error()` failed]"),
          error.domain, error.code, error.message);

    this->m_reset_clear = true;
    ::mongoc_client_set_error_api(this->m_mongo, MONGOC_ERROR_API_VERSION_2);
  }

Mongo_Connection::
~Mongo_Connection()
  {
  }

bool
Mongo_Connection::
reset() noexcept
  {
    // Discard the current reply and cursor.
    ::bson_reinit(this->m_reply);
    this->m_cursor.reset();
    this->m_reset_clear = false;

    // Check whether the server is still active. This is a blocking function.
    auto server = ::mongoc_client_select_server(this->m_mongo, false, nullptr, nullptr);
    bool no_available_server = !server;
    ::mongoc_server_description_destroy(server);
    if(no_available_server)
      return false;

    this->m_reset_clear = true;
    return true;
  }

void
Mongo_Connection::
execute(const scoped_bson& cmd)
  {
    // Discard the current reply and cursor.
    ::bson_reinit(this->m_reply);
    this->m_cursor.reset();
    this->m_reset_clear = false;

    // Execute the command. `mongoc_client_command_with_opts()` always recreate
    // the reply object, so destroy it first.
    ::bson_error_t error;
    ::bson_destroy(this->m_reply);
    if(!::mongoc_client_command_with_opts(this->m_mongo, this->m_db.c_str(), cmd, nullptr,
                                          nullptr, this->m_reply, &error))
      POSEIDON_THROW((
          "Could not execute Mongo command: ERROR $1.$2: $3",
          "[`mongoc_client_command_with_opts()` failed]"),
          error.domain, error.code, error.message);
  }

bool
Mongo_Connection::
fetch_reply(scoped_bson& output)
  {
    ::bson_reinit(output);

    if(!this->m_cursor) {
      // There are two possibilities: Either the reply has not been parsed yet,
      // or the reply does not contain a cursor.
      // First, check whether the reply has been parsed.
      if(bson_empty(static_cast<const ::bson_t*>(this->m_reply)))
        return false;

      if(!::bson_has_field(this->m_reply, "cursor")) {
        // If the reply does not contain a cursor, move it into `output`.
        ::bson_destroy(output);
        ::bson_steal(output, this->m_reply);
        ::bson_init(this->m_reply);
        return true;
      }

      // Create the cursor. `mongoc_cursor_new_from_command_reply_with_opts()`
      // destroys the reply object, so it has to be recreated afterwards.
      this->m_cursor.reset(::mongoc_cursor_new_from_command_reply_with_opts(
                                          this->m_mongo, this->m_reply, nullptr));
      ROCKET_ASSERT(this->m_cursor);
      ::bson_init(this->m_reply);
    }

    // Get the next object from the cursor.
    const ::bson_t* result;
    if(!::mongoc_cursor_next(this->m_cursor, &result)) {
      // Check whether an error has been stored in the cursor.
      // https://mongoc.org/libmongoc/current/mongoc_cursor_next.html
      ::bson_error_t error;
      if(!::mongoc_cursor_error(this->m_cursor, &error))
        return false;

      POSEIDON_THROW((
          "Could not fetch result from Mongo server: ERROR $1.$2: $3",
          "[`mongoc_cursor_next()` failed]"),
          error.domain, error.code, error.message);
    }

    // Make a copy of the result object.
    ::bson_destroy(output);
    ::bson_copy_to(result, output);
    return true;
  }

}  // namespace poseidon
