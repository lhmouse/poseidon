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
    const auto server_guard = ::rocket::make_unique_handle(server, ::mongoc_server_description_destroy);
    if(!server)
      return false;

    this->m_reset_clear = true;
    return true;
  }

void
Mongo_Connection::
execute(const Mongo_Document& cmd)
  {
    // Discard the current reply and cursor.
    ::bson_reinit(this->m_reply);
    this->m_cursor.reset();
    this->m_reset_clear = false;

    // Pack the command into a BSON object. The `bson_t` type has a HUGE alignment
    // of 128, which prevents use of standard containers.
    struct rb_ctx
      {
        rb_ctx* prev = nullptr;
        ::bson_t* parent = nullptr;
        const Mongo_Array* arr = nullptr;
        const Mongo_Document* doc = nullptr;
        size_t index = 0;
        ::bson_t child;  // uninitialized at top
      };

    ::rocket::unique_ptr<rb_ctx, void (rb_ctx*&)> rb_top(new rb_ctx,
           *[](rb_ctx*& top) {
             while(top)
               delete ::std::exchange(top, top->prev);
           });

    scoped_bson bson_cmd;
    rb_top->parent = bson_cmd;
    rb_top->doc = &cmd;
    bool bson_success = true;

    while(rb_top && bson_success) {
      ::rocket::ascii_numput key_nump;
      const char* key;
      int key_len;
      const Mongo_Value* value;

      if(rb_top->arr) {
        if(rb_top->index >= rb_top->arr->size()) {
          // Leave this array.
          ::rocket::unique_ptr<rb_ctx> rb_old_top(rb_top.release());
          rb_top.reset(rb_old_top->prev);
          bson_success = !rb_top || ::bson_append_array_end(rb_top->parent, &(rb_top->child));
          continue;
        }

        // Get an element.
        key_nump.put_DU(rb_top->index);
        key = key_nump.data();
        key_len = static_cast<int>(key_nump.size());
        value = &(rb_top->arr->at(rb_top->index));
        rb_top->index ++;
      }
      else {
        if(rb_top->index >= rb_top->doc->size()) {
          // Leave this document.
          ::rocket::unique_ptr<rb_ctx> rb_old_top(rb_top.release());
          rb_top.reset(rb_old_top->prev);
          bson_success = !rb_top || ::bson_append_document_end(rb_top->parent, &(rb_top->child));
          continue;
        }

        // Get an element.
        key = rb_top->doc->at(rb_top->index).first.c_str();
        key_len = clamp_cast<int>(rb_top->doc->at(rb_top->index).first.size(), 0, INT_MAX);
        value = &(rb_top->doc->at(rb_top->index).second);
        rb_top->index ++;
      }

      if(value->is_null()) {
        // null
        bson_success = ::bson_append_null(rb_top->parent, key, key_len);
      }
      else if(value->is_boolean()) {
        // true or false
        bson_success = ::bson_append_bool(rb_top->parent, key, key_len, value->as_boolean());
      }
      else if(value->is_integer()) {
        // integer
        bson_success = ::bson_append_int64(rb_top->parent, key, key_len, value->as_integer());
      }
      else if(value->is_double()) {
        // double
        bson_success = ::bson_append_double(rb_top->parent, key, key_len, value->as_double());
      }
      else if(value->is_utf8()) {
        // string
        bson_success = (value->utf8_size() <= INT_MAX)
                       && ::bson_append_utf8(rb_top->parent, key, key_len, value->utf8_data(),
                                             static_cast<int>(value->utf8_size()));
      }
      else if(value->is_binary()) {
        // binary
        bson_success = (value->binary_size() <= INT_MAX)
                       && ::bson_append_binary(rb_top->parent, key, key_len, BSON_SUBTYPE_BINARY,
                               value->binary_data(), static_cast<uint32_t>(value->binary_size()));
      }
      else if(value->is_array()) {
        // index-value pairs
        ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
        bson_success = ::bson_append_array_begin(rb_top->parent, key, key_len, &(rb_top->child));
        if(!bson_success)
          break;

        rb_next->parent = &(rb_top->child);
        rb_next->arr = &(value->as_array());
        rb_next->prev = rb_top.release();
        rb_top.reset(rb_next.release());
      }
      else if(value->is_document()) {
        // key-value pairs
        ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
        bson_success = ::bson_append_document_begin(rb_top->parent, key, key_len, &(rb_top->child));
        if(!bson_success)
          break;

        rb_next->parent = &(rb_top->child);
        rb_next->doc = &(value->as_document());
        rb_next->prev = rb_top.release();
        rb_top.reset(rb_next.release());
      }
      else if(value->is_oid()) {
        // object id
        bson_success = ::bson_append_oid(rb_top->parent, key, key_len, &(value->as_oid()));
      }
      else if(value->is_datetime()) {
        // timestamp
        bson_success = ::bson_append_date_time(rb_top->parent, key, key_len,
                                               static_cast<int64_t>(value->as_time_t()) * 1000);
      }
      else
        POSEIDON_THROW(("Unhandled `Mongo_Value`: $1"), *value);
    }

    if(!bson_success)
      POSEIDON_THROW(("Failed to compose BSON command"));

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
  }

POSEIDON_VISIBILITY_HIDDEN
void
Mongo_Connection::
do_set_document_from_bson(Mongo_Document& output, const ::bson_t* input) const
  {
    // Unpack a BSON object. The `bson_t` type has a HUGE alignment of 128, which
    // prevents use of standard containers.
    struct rb_ctx
      {
        rb_ctx* prev = nullptr;
        Mongo_Array* arr = nullptr;
        Mongo_Document* doc = nullptr;
        ::bson_iter_t iter;
      };

    ::rocket::unique_ptr<rb_ctx, void (rb_ctx*&)> rb_top(new rb_ctx,
           *[](rb_ctx*& top) {
             while(top)
               delete ::std::exchange(top, top->prev);
           });

    rb_top->doc = &output;
    bool bson_success = ::bson_iter_init(&(rb_top->iter), input);

    while(rb_top && bson_success) {
      if(!::bson_iter_next(&(rb_top->iter))) {
        // Leave this value.
        ::rocket::unique_ptr<rb_ctx> rb_old_top(rb_top.release());
        rb_top.reset(rb_old_top->prev);
        continue;
      }

      uint32_t type = ::bson_iter_type(&(rb_top->iter));
      switch(type)
        {
        case BSON_TYPE_NULL:
          {
            if(rb_top->arr)
              rb_top->arr->emplace_back(nullptr);
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), nullptr);
          }
          break;

        case BSON_TYPE_BOOL:
          {
            bool value = ::bson_iter_bool_unsafe(&(rb_top->iter));

            if(rb_top->arr)
              rb_top->arr->emplace_back(value);
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), value);
          }
          break;

        case BSON_TYPE_INT32:
          {
            int64_t value = ::bson_iter_int32_unsafe(&(rb_top->iter));

            if(rb_top->arr)
              rb_top->arr->emplace_back(value);
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), value);
          }
          break;

        case BSON_TYPE_INT64:
          {
            int64_t value = ::bson_iter_int64_unsafe(&(rb_top->iter));

            if(rb_top->arr)
              rb_top->arr->emplace_back(value);
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), value);
          }
          break;

        case BSON_TYPE_DOUBLE:
          {
            double value = ::bson_iter_double_unsafe(&(rb_top->iter));

            if(rb_top->arr)
              rb_top->arr->emplace_back(value);
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), value);
          }
          break;

        case BSON_TYPE_UTF8:
          {
            uint32_t len;
            const char* str = ::bson_iter_utf8(&(rb_top->iter), &len);
            cow_string value(str, len);

            if(rb_top->arr)
              rb_top->arr->emplace_back(move(value));
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), move(value));
          }
          break;

        case BSON_TYPE_BINARY:
          {
            uint32_t len;
            const uint8_t* ptr;
            ::bson_iter_binary(&(rb_top->iter), nullptr, &len, &ptr);
            Mongo_Binary value(ptr, ptr + len);

            if(rb_top->arr)
              rb_top->arr->emplace_back(move(value));
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), move(value));
          }
          break;

        case BSON_TYPE_ARRAY:
          {
            uint32_t len;
            const uint8_t* ptr;
            ::bson_iter_array(&(rb_top->iter), &len, &ptr);

            ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
            bson_success = ::bson_iter_init_from_data(&(rb_next->iter), ptr, len);
            if(!bson_success)
              break;

            if(rb_top->arr)
              rb_next->arr = &(rb_top->arr->emplace_back().mut_array());
            else
              rb_next->arr = &(rb_top->doc->emplace_back(
                                    ::bson_iter_key_unsafe(&(rb_top->iter)),
                                     nullptr).second.mut_array());

            rb_next->prev = rb_top.release();
            rb_top.reset(rb_next.release());
          }
          break;

        case BSON_TYPE_DOCUMENT:
          {
            uint32_t len;
            const uint8_t* ptr;
            ::bson_iter_document(&(rb_top->iter), &len, &ptr);

            ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
            bson_success = ::bson_iter_init_from_data(&(rb_next->iter), ptr, len);
            if(!bson_success)
              break;

            if(rb_top->arr)
              rb_next->doc = &(rb_top->arr->emplace_back().mut_document());
            else
              rb_next->doc = &(rb_top->doc->emplace_back(
                                    ::bson_iter_key_unsafe(&(rb_top->iter)),
                                     nullptr).second.mut_document());

            rb_next->prev = rb_top.release();
            rb_top.reset(rb_next.release());
          }
          break;

        case BSON_TYPE_OID:
          {
            const ::bson_oid_t* poid = ::bson_iter_oid(&(rb_top->iter));

            if(rb_top->arr)
              rb_top->arr->emplace_back(*poid);
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), *poid);
          }
          break;

        case BSON_TYPE_DATE_TIME:
          {
            milliseconds ms(::bson_iter_date_time(&(rb_top->iter)));
            system_time sys_tm(ms);

            if(rb_top->arr)
              rb_top->arr->emplace_back(sys_tm);
            else
              rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), sys_tm);
          }
          break;
        }
    }
  }

bool
Mongo_Connection::
fetch_reply(Mongo_Document& output)
  {
    output.clear();

    if(!this->m_cursor) {
      // There are two possibilities: Either the reply has not been parsed yet,
      // or the reply does not contain a cursor.
      // First, check whether the reply has been parsed.
      if(this->m_reply.size() <= 5)
        return false;

      if(!::bson_has_field(this->m_reply, "cursor")) {
        // If the reply does not contain a cursor, copy it directly to `output`,
        // then clear it.
        this->do_set_document_from_bson(output, this->m_reply);
        ::bson_reinit(this->m_reply);
        return true;
      }

      // Create the cursor. `mongoc_cursor_new_from_command_reply_with_opts()`
      // destroys the reply object, so it has to be recreated afterwards.
      this->m_cursor.reset(::mongoc_cursor_new_from_command_reply_with_opts(
                                            this->m_mongo, this->m_reply, nullptr));
      ROCKET_ASSERT(this->m_cursor);
      ::bson_init(this->m_reply);
    }

    // Get the next document from the reply cursor.
    ::bson_error_t error;
    const ::bson_t* result;
    if(!::mongoc_cursor_next(this->m_cursor, &result)) {
      // Check whether an error has been stored in the cursor.
      if(!::mongoc_cursor_error(this->m_cursor, &error))
        return false;

      POSEIDON_THROW((
          "Could not fetch result from Mongo server: ERROR $1.$2: $3",
          "[`mongoc_cursor_next()` failed]"),
          error.domain, error.code, error.message);
    }

    // Make a copy.
    this->do_set_document_from_bson(output, result);
    return true;
  }

}  // namespace poseidon
