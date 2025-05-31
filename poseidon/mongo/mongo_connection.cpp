// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_connection.hpp"
#include "../utils.hpp"
namespace poseidon {

Mongo_Connection::
Mongo_Connection(cow_stringR service_uri, cow_stringR password, uint32_t password_mask)
  {
    this->m_service_uri = service_uri;
    this->m_reply_available = false;
    this->m_reset_clear = true;

    // Parse the service URI.
    auto full_uri = "mongodb://" + service_uri;
    ::bson_error_t error;
    uniptr_mongoc_uri uri;
    if(!uri.reset(::mongoc_uri_new_with_error(full_uri.c_str(), &error)))
      POSEIDON_THROW((
          "Invalid Mongo service URI: ERROR $1.$2: $3",
          "[`mongoc_uri_new_with_error()` failed]"),
          error.domain, error.code, error.message);

    ::mongoc_uri_set_option_as_utf8(uri, MONGOC_URI_COMPRESSORS, "zlib");
    this->m_db.assign(::mongoc_uri_get_database(uri));

    // Unmask the password which is sensitive data, so erasure shall be ensured.
    Unmasked_Password real_password(password, password_mask);
    if(!::mongoc_uri_set_password(uri, real_password.c_str()))
      POSEIDON_THROW((
          "Invalid Mongo password (maybe it's not valid UTF-8?)",
          "[`mongoc_uri_set_password()` failed]"));

    // Create the client object. This does not initiate the connection.
    if(!this->m_mongo.reset(::mongoc_client_new_from_uri_with_error(uri, &error)))
      POSEIDON_THROW((
          "Could not create Mongo client object: ERROR $1.$2: $3",
          "[`mongoc_client_new_from_uri_with_error()` failed]"),
          error.domain, error.code, error.message);

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
    this->m_cursor.reset();
    this->m_reply_available = false;
    this->m_reset_clear = false;

    // Check whether the server is still active. This is a blocking function.
    auto servers = make_unique_handle(::mongoc_client_select_server(this->m_mongo,
                                                            false, nullptr, nullptr),
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
        ::bson_t* parent_bson;
        decltype(::bson_append_array_end)* append_end_fn;
        const Mongo_Value* parent;
        size_t parent_rpos;
        ::bson_t temp;
      };

    scoped_bson bson_cmd;
    ::std::forward_list<xFrame> stack;
    bool success = true;
    size_t top_rpos = cmd.size();
    ::bson_t* pbson = bson_cmd;
    const Mongo_Array* top_a = nullptr;
    const Mongo_Document* top_o = &cmd;

  do_unpack_loop_:
    while(success && (top_rpos != 0)) {
      ::rocket::ascii_numput array_key_nump;
      const char* key;
      int key_len;
      const Mongo_Value* pval;
      int32_t int32;
      milliseconds dur;

      if(top_a) {
        // array; keys are subscripts as decimal strings.
        size_t pos = top_a->size() - top_rpos;
        top_rpos --;
        array_key_nump.put_DU(pos);
        key = array_key_nump.data();
        key_len = static_cast<int>(array_key_nump.size());
        pval = &(top_a->at(pos));
      }
      else {
        // documents
        size_t pos = top_o->size() - top_rpos;
        top_rpos --;
        key = top_o->at(pos).first.data();
        key_len = clamp_cast<int>(top_o->at(pos).first.ssize(), 0, INT_MAX);
        pval = &(top_o->at(pos).second);
      }

      switch(pval->type())
        {
        case mongo_value_null:
          success = ::bson_append_null(pbson, key, key_len);
          break;

        case mongo_value_boolean:
          success = ::bson_append_bool(pbson, key, key_len, pval->as_boolean());
          break;

        case mongo_value_integer:
          int32 = static_cast<int32_t>(pval->as_integer());
          success = (int32 == pval->as_integer())
                    ? ::bson_append_int32(pbson, key, key_len, int32)
                    : ::bson_append_int64(pbson, key, key_len, pval->as_integer());
          break;

        case mongo_value_double:
          success = ::bson_append_double(pbson, key, key_len, pval->as_double());
          break;

        case mongo_value_utf8:
          success = (pval->as_utf8_length() <= INT_MAX)
                    && ::bson_append_utf8(pbson, key, key_len, pval->as_utf8_c_str(),
                                          static_cast<int>(pval->as_utf8_length()));
          break;

        case mongo_value_binary:
          success = (pval->as_binary_size() <= INT_MAX)
                    && ::bson_append_binary(pbson, key, key_len, BSON_SUBTYPE_BINARY,
                                            pval->as_binary_data(),
                                            static_cast<uint32_t>(pval->as_binary_size()));
          break;

        case mongo_value_array:
        case mongo_value_document:
          {
            auto& frm = stack.emplace_front();
            frm.parent_bson = pbson;
            frm.parent = pval;
            frm.parent_rpos = top_rpos;

            switch(pval->m_stor.index())
              {
              case mongo_value_array:
                top_rpos = pval->as_array().size();
                success = ::bson_append_array_begin(pbson, key, key_len, &(frm.temp));
                frm.append_end_fn = ::bson_append_array_end;
                break;

              case mongo_value_document:
                top_rpos = pval->as_document().size();
                success = ::bson_append_document_begin(pbson, key, key_len, &(frm.temp));
                frm.append_end_fn = ::bson_append_document_end;
                break;
              }

            if(success) {
              // open
              pbson = &(frm.temp);
              top_a = pval->m_stor.ptr<Mongo_Array>();
              top_o = pval->m_stor.ptr<Mongo_Document>();
              goto do_unpack_loop_;
            }

            // invalid; shouldn't happen, but be tolerant anyway.
            top_rpos = frm.parent_rpos;
            stack.pop_front();
          }
          break;

        case mongo_value_oid:
          success = ::bson_append_oid(pbson, key, key_len, &(pval->as_oid()));
          break;

        case mongo_value_datetime:
          dur = duration_cast<milliseconds>(pval->as_system_time() - system_time());
          success = ::bson_append_date_time(pbson, key, key_len, dur.count());
          break;

        default:
          ASTERIA_TERMINATE(("Corrupted value type `$1`"), pval->m_stor.index());
        }
    }

    if(success && !stack.empty()) {
      auto& frm = stack.front();
      top_rpos = frm.parent_rpos;
      pbson = frm.parent_bson;
      top_a = frm.parent->m_stor.ptr<Mongo_Array>();
      top_o = frm.parent->m_stor.ptr<Mongo_Document>();
      success = frm.append_end_fn(pbson, &(frm.temp));
      stack.pop_front();
      goto do_unpack_loop_;
    }

    if(!success)
      POSEIDON_THROW(("Failed to compose BSON command"));

    ::bson_error_t error;
    if(!::bson_validate_with_error(bson_cmd, BSON_VALIDATE_UTF8, &error))
      POSEIDON_THROW((
          "Invalid BSON command: $1",
          "[`bson_validate_with_error()` failed]"),
          error.message);

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

    const ::bson_t* bson_output;
    if(!::mongoc_cursor_next(this->m_cursor, &bson_output)) {
      // An error may have been stored in the cursor.
      ::bson_error_t error;
      if(::mongoc_cursor_error(this->m_cursor, &error))
        POSEIDON_THROW((
            "Could not fetch result from Mongo server: ERROR $1.$2: $3",
            "[`mongoc_cursor_next()` failed]"),
            error.domain, error.code, error.message);
    }

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
        Mongo_Value* parent;
        ::bson_iter_t parent_iter;
      };

    ::std::forward_list<xFrame> stack;
    Mongo_Array* top_a = nullptr;
    Mongo_Document* top_o = &output;

    ::bson_iter_t top_iter;
    if(!::bson_iter_init(&top_iter, bson_output))
      POSEIDON_THROW(("Failed to parse BSON reply from server"));

  do_pack_loop_:
    while(::bson_iter_next(&top_iter)) {
      Mongo_Value* pval;
      const char* str;
      const unsigned char* bytes;
      uint32_t type, len;
      milliseconds dur;

      if(top_a) {
        // array
        pval = &(top_a->emplace_back());
      }
      else {
        // document
        str = reinterpret_cast<const char*>(top_iter.raw + top_iter.key);
        len = top_iter.d1 - top_iter.key - 1;

        auto& pair = top_o->emplace_back();
        pair.first.append(str, len);
        pval = &(pair.second);
      }

      type = ::bson_iter_type_unsafe(&top_iter);
      switch(type)
        {
        case BSON_TYPE_NULL:
          break;

        case BSON_TYPE_BOOL:
          pval->mut_boolean() = ::bson_iter_bool_unsafe(&top_iter);
          break;

        case BSON_TYPE_INT32:
          pval->mut_integer() = ::bson_iter_int32_unsafe(&top_iter);
          break;

        case BSON_TYPE_INT64:
          pval->mut_integer() = ::bson_iter_int64_unsafe(&top_iter);
          break;

        case BSON_TYPE_DOUBLE:
          pval->mut_double() = ::bson_iter_double_unsafe(&top_iter);
          break;

        case BSON_TYPE_UTF8:
          str = ::bson_iter_utf8(&top_iter, &len);
          pval->mut_utf8().append(str, len);
          break;

        case BSON_TYPE_BINARY:
          ::bson_iter_binary(&top_iter, nullptr, &len, &bytes);
          pval->mut_binary().append(bytes, len);
          break;

        case BSON_TYPE_ARRAY:
        case BSON_TYPE_DOCUMENT:
          switch(type)
            {
            case BSON_TYPE_ARRAY:
              ::bson_iter_array(&top_iter, &len, &bytes);
              pval->mut_array();
              break;

            case BSON_TYPE_DOCUMENT:
              ::bson_iter_document(&top_iter, &len, &bytes);
              pval->mut_document();
              break;
            }

          if(len > 5) {
            auto& frm = stack.emplace_front();
            frm.parent = pval;
            frm.parent_iter = top_iter;

            if(::bson_iter_init_from_data(&top_iter, bytes, len)) {
              // open
              top_a = pval->m_stor.mut_ptr<Mongo_Array>();
              top_o = pval->m_stor.mut_ptr<Mongo_Document>();
              goto do_pack_loop_;
            }

            // invalid; shouldn't happen, but be tolerant anyway.
            top_iter = frm.parent_iter;
            stack.pop_front();
          }
          break;

        case BSON_TYPE_OID:
          pval->mut_oid() = *::bson_iter_oid_unsafe(&top_iter);
          break;

        case BSON_TYPE_DATE_TIME:
          dur = milliseconds(::bson_iter_date_time(&top_iter));
          pval->mut_datetime() = system_time() + dur;
          break;
        }
    }

    if(!stack.empty()) {
      auto& frm = stack.front();
      top_a = frm.parent->m_stor.mut_ptr<Mongo_Array>();
      top_o = frm.parent->m_stor.mut_ptr<Mongo_Document>();
      top_iter = frm.parent_iter;
      stack.pop_front();
      goto do_pack_loop_;
    }

    return true;
  }

}  // namespace poseidon
