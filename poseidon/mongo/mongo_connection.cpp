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
    auto server = ::mongoc_client_select_server(this->m_mongo, false, nullptr, nullptr);
    const auto server_guard = ::rocket::make_unique_handle(server, ::mongoc_server_description_destroy);
    if(!server)
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
    // Pack the command into a BSON object. The `bson_t` type has a HUGE
    // alignment of 128, which prevents use of standard containers.
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

      switch(value->type())
        {
        case mongo_value_null:
          bson_success = ::bson_append_null(rb_top->parent, key, key_len);
          break;

        case mongo_value_boolean:
          bson_success = ::bson_append_bool(rb_top->parent, key, key_len, value->as_boolean());
          break;

        case mongo_value_integer:
          bson_success = ::bson_append_int64(rb_top->parent, key, key_len, value->as_integer());
          break;

        case mongo_value_double:
          bson_success = ::bson_append_double(rb_top->parent, key, key_len, value->as_double());
          break;

        case mongo_value_utf8:
          bson_success = (value->utf8_size() <= INT_MAX)
                     && ::bson_append_utf8(rb_top->parent, key, key_len,
                                           value->utf8_data(), static_cast<int>(value->utf8_size()));
          break;

        case mongo_value_binary:
          bson_success = (value->binary_size() <= INT_MAX)
                 && ::bson_append_binary(rb_top->parent, key, key_len, BSON_SUBTYPE_BINARY,
                                  value->binary_data(), static_cast<uint32_t>(value->binary_size()));
          break;

        case mongo_value_array:
          {
            ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
            bson_success = ::bson_append_array_begin(rb_top->parent, key, key_len, &(rb_top->child));
            if(!bson_success)
              break;

            rb_next->parent = &(rb_top->child);
            rb_next->arr = &(value->as_array());
            rb_next->prev = rb_top.release();
            rb_top.reset(rb_next.release());
          }
          break;

        case mongo_value_document:
          {
            ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
            bson_success = ::bson_append_document_begin(rb_top->parent, key, key_len, &(rb_top->child));
            if(!bson_success)
              break;

            rb_next->parent = &(rb_top->child);
            rb_next->doc = &(value->as_document());
            rb_next->prev = rb_top.release();
            rb_top.reset(rb_next.release());
          }
          break;

        case mongo_value_oid:
          bson_success = ::bson_append_oid(rb_top->parent, key, key_len, &(value->as_oid()));
          break;

        case mongo_value_datetime:
          bson_success = ::bson_append_date_time(rb_top->parent, key, key_len,
                     time_point_cast<milliseconds>(value->as_system_time()).time_since_epoch().count());
          break;

        default:
          POSEIDON_THROW(("Unknown Mongo value type: $1"), *value);
        }
    }

    if(!bson_success)
      POSEIDON_THROW(("Failed to compose BSON command"));

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
    ::bson_error_t error;
    const ::bson_t* bson_output;
    bool fetched = ::mongoc_cursor_next(this->m_cursor, &bson_output);

    if(!fetched && ::mongoc_cursor_error(this->m_cursor, &error))
      POSEIDON_THROW((
          "Could not fetch result from Mongo server: ERROR $1.$2: $3",
          "[`mongoc_cursor_next()` failed]"),
          error.domain, error.code, error.message);

    if(!fetched)
      return nullptr;

    // Return the result inside the cursor.
    return bson_output;
  }

bool
Mongo_Connection::
fetch_reply(Mongo_Document& output)
  {
    output.clear();

    const ::bson_t* bson_output = this->fetch_reply_bson_opt();
    if(!bson_output)
      return false;

    // Unpack a BSON object. The `bson_t` type has a HUGE alignment of 128,
    // which prevents use of standard containers.
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
    bool bson_success = ::bson_iter_init(&(rb_top->iter), bson_output);

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
            Mongo_Value* value;

            ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
            bson_success = ::bson_iter_init_from_data(&(rb_next->iter), ptr, len);
            if(!bson_success)
              break;

            if(rb_top->arr)
              value = &(rb_top->arr->emplace_back());
            else
              value = &(rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), nullptr).second);

            rb_next->arr = &(value->mut_array());
            rb_next->prev = rb_top.release();
            rb_top.reset(rb_next.release());
          }
          break;

        case BSON_TYPE_DOCUMENT:
          {
            uint32_t len;
            const uint8_t* ptr;
            ::bson_iter_document(&(rb_top->iter), &len, &ptr);
            Mongo_Value* value;

            ::rocket::unique_ptr<rb_ctx> rb_next(new rb_ctx);
            bson_success = ::bson_iter_init_from_data(&(rb_next->iter), ptr, len);
            if(!bson_success)
              break;

            if(rb_top->arr)
              value = &(rb_top->arr->emplace_back());
            else
              value = &(rb_top->doc->emplace_back(::bson_iter_key_unsafe(&(rb_top->iter)), nullptr).second);

            rb_next->doc = &(value->mut_document());
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

    if(!bson_success)
      POSEIDON_THROW(("Failed to parse BSON reply"));

    // Return the result into `output`.
    return true;
  }

}  // namespace poseidon
