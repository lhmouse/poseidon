// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_connection.hpp"
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

    ::mongoc_client_set_error_api(this->m_mongo, MONGOC_ERROR_API_VERSION_2);
    this->m_reply_available = false;
    this->m_reset_clear = true;
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
    this->m_cursor.reset();
    this->m_reply_available = false;
    this->m_reset_clear = false;

    // Check whether the server is still active. This is a blocking function.
    auto servers = ::rocket::make_unique_handle(
             ::mongoc_client_select_server(this->m_mongo, false, nullptr, nullptr),
                   ::mongoc_server_description_destroy);
    if(!servers)
      return false;

    this->m_reset_clear = true;
    return true;
  }

void
Mongo_Connection::
execute_bson(const ::bson_t* bson_cmd)
  {
    if(!bson_cmd)
      POSEIDON_THROW(("Null BSON pointer"));

    // Discard the current reply and cursor.
    this->m_cursor.reset();
    this->m_reply_available = false;
    this->m_reset_clear = false;

    // Execute the command. `mongoc_client_command_with_opts()` always recreate
    // the reply object, so destroy it first.
    ::bson_error_t error;
    ::bson_destroy(this->m_reply);
    if(!::mongoc_client_command_with_opts(this->m_mongo, this->m_db.c_str(), bson_cmd,
                                          nullptr, nullptr, this->m_reply, &error))
      POSEIDON_THROW((
          "Could not execute Mongo command: ERROR $1.$2: $3",
          "[`mongoc_client_command_with_opts()` failed]"),
          error.domain, error.code, error.message);

    // This will be checked in `fetch_reply_bson_opt()`.
    this->m_reply_available = true;
  }

void
Mongo_Connection::
execute(const Mongo_Document& cmd)
  {
    // Pack the command into a BSON object.
    struct xFrame
      {
        const Mongo_Array* psa;
        const Mongo_Document* pso;
        size_t pos;
        size_t size;
        ::bson_t temp;
      };

    scoped_bson bson_cmd;
    list<xFrame> stack;
    const Mongo_Array* pval_a = nullptr;
    const Mongo_Document* pval_o = &cmd;
    size_t pos = SIZE_MAX;
    size_t size = cmd.size();

  do_unpack_loop_:
    while(++ pos != size)
    ;




    return this->execute_bson(bson_cmd);
  }

const ::bson_t*
Mongo_Connection::
fetch_reply_bson_opt()
  {
    if(!this->m_cursor) {
      // There are two possibilities: Either the reply has not been parsed yet,
      // or the reply does not contain a cursor.
      if(!this->m_reply_available)
        return nullptr;

      if(!::bson_has_field(this->m_reply, "cursor")) {
        // If the reply contains no cursor, assign it directly to `bson_output`.
        this->m_reply_available = false;
        return this->m_reply;
      }

      // Create the cursor. `mongoc_cursor_new_from_command_reply_with_opts()`
      // destroys the reply object, so it has to be recreated afterwards.
      this->m_cursor.reset(::mongoc_cursor_new_from_command_reply_with_opts(
                                        this->m_mongo, this->m_reply, nullptr));
      ::bson_init(this->m_reply);
      this->m_reply_available = false;
    }

    // Get the next document from the reply cursor.
    const ::bson_t* bson_output;
    bool fetched = ::mongoc_cursor_next(this->m_cursor, &bson_output);

    ::bson_error_t error;
    if(!fetched && ::mongoc_cursor_error(this->m_cursor, &error))
      POSEIDON_THROW((
          "Could not fetch result from Mongo server: ERROR $1.$2: $3",
          "[`mongoc_cursor_next()` failed]"),
          error.domain, error.code, error.message);

    if(!fetched)
      return nullptr;

    return bson_output;
  }

bool
Mongo_Connection::
fetch_reply(Mongo_Document& output)
  {
    output.clear();

    const auto bson_output = this->fetch_reply_bson_opt();
    if(!bson_output)
      return false;

    // Parse the reply and store the result into `output`.
    struct xFrame
      {
        Mongo_Array* target_a;
        Mongo_Document* target_o;
        Mongo_Value vs;
        ::bson_iter_t parent_iter;
      };

    list<xFrame> stack;
    Mongo_Array* pval_a = nullptr;
    Mongo_Document* pval_o = &output;
    ::bson_iter_t top_iter;

    if(!::bson_iter_init(&top_iter, bson_output))
      POSEIDON_THROW(("Failed to parse BSON reply from server"));

#define do_reply_output_  \
    (pval_a ? pval_a->emplace_back()  \
       : pval_o->emplace_back(::bson_iter_key_unsafe(&top_iter), nullptr).second)

    uint32_t size;
    const char* str;
    const unsigned char* bytes;
    int64_t num;
    ::bson_iter_t child_iter;

  do_pack_loop_:
    while(::bson_iter_next(&top_iter))
      switch(static_cast<uint32_t>(::bson_iter_type(&top_iter)))
        {
        case BSON_TYPE_NULL:
          do_reply_output_;
          break;

        case BSON_TYPE_BOOL:
          do_reply_output_.mut_boolean() = ::bson_iter_bool_unsafe(&top_iter);
          break;

        case BSON_TYPE_INT32:
          do_reply_output_.mut_integer() = ::bson_iter_int32_unsafe(&top_iter);
          break;

        case BSON_TYPE_INT64:
          do_reply_output_.mut_integer() = ::bson_iter_int64_unsafe(&top_iter);
          break;

        case BSON_TYPE_DOUBLE:
          do_reply_output_.mut_double() = ::bson_iter_double_unsafe(&top_iter);
          break;

        case BSON_TYPE_UTF8:
          str = ::bson_iter_utf8(&top_iter, &size);
          do_reply_output_.mut_utf8().assign(str, size);
          break;

        case BSON_TYPE_ARRAY:
          ::bson_iter_array(&top_iter, &size, &bytes);
          if(::bson_iter_init_from_data(&child_iter, bytes, size)) {
            // open
            auto& frm = stack.emplace_back();
            frm.target_a = pval_a;
            frm.target_o = pval_o;
            pval_a = &(frm.vs.mut_array());
            pval_o = nullptr;
            frm.parent_iter = top_iter;
            top_iter = child_iter;
            goto do_pack_loop_;
          }

          // empty
          do_reply_output_.mut_array();
          break;

        case BSON_TYPE_DOCUMENT:
          ::bson_iter_document(&top_iter, &size, &bytes);
          if(::bson_iter_init_from_data(&child_iter, bytes, size)) {
            // open
            auto& frm = stack.emplace_back();
            frm.target_a = pval_a;
            frm.target_o = pval_o;
            pval_a = nullptr;
            pval_o = &(frm.vs.mut_document());
            frm.parent_iter = top_iter;
            top_iter = child_iter;
            goto do_pack_loop_;
          }

          // empty
          do_reply_output_.mut_document();
          break;

        case BSON_TYPE_BINARY:
          ::bson_iter_binary(&top_iter, nullptr, &size, &bytes);
          do_reply_output_.mut_binary().assign(bytes, size);
          break;

        case BSON_TYPE_OID:
          do_reply_output_.mut_oid() = *::bson_iter_oid_unsafe(&top_iter);
          break;

        case BSON_TYPE_DATE_TIME:
          num = ::bson_iter_date_time(&top_iter);
          do_reply_output_.mut_datetime() = system_time() + milliseconds(num);
          break;
        }

    if(!stack.empty()) {
      auto& frm = stack.back();

      // move the sub-object
      if(frm.target_a)
        frm.target_a->emplace_back(move(frm.vs));
      else
        frm.target_o->emplace_back(::bson_iter_key_unsafe(&(frm.parent_iter)),
                                   move(frm.vs));

      // close
      pval_a = frm.target_a;
      pval_o = frm.target_o;
      top_iter = frm.parent_iter;
      stack.pop_back();
      goto do_pack_loop_;
    }

    return true;
  }

}  // namespace poseidon
