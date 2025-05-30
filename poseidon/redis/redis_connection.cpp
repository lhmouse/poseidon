// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_connection.hpp"
#include "redis_value.hpp"
#include "../utils.hpp"
#include <http_parser.h>
namespace poseidon {

Redis_Connection::
Redis_Connection(cow_stringR service_uri, cow_stringR password, uint32_t password_mask)
  {
    this->m_service_uri = service_uri;
    this->m_password = password;
    this->m_password_mask = password_mask;
    this->m_connected = false;
    this->m_reset_clear = true;
  }

Redis_Connection::
~Redis_Connection()
  {
  }

bool
Redis_Connection::
reset() noexcept
  {
    if(!this->m_connected)
      return true;

    // Discard the current reply.
    this->m_reply.reset();
    this->m_reset_clear = false;

    // Check whether an error has occurred.
    if(this->m_redis->err != 0)
      return false;

    this->m_reset_clear = true;
    return true;
  }

void
Redis_Connection::
execute(const cow_string* cmds, size_t ncmds)
  {
    if(!cmds || (ncmds == 0))
      POSEIDON_THROW(("Empty Redis command"));

    if(!this->m_connected) {
      // Parse the service URI in a hacky way.
      auto full_uri = "redis://" + this->m_service_uri;
      ::http_parser_url uri_hp;
      ::http_parser_url_init(&uri_hp);
      if(::http_parser_parse_url(full_uri.c_str(), full_uri.size(), false, &uri_hp) != 0)
        POSEIDON_THROW((
            "Invalid Redis service URI `$1`",
            "[`http_parser_parse_url()` failed]"),
            this->m_service_uri);

      const char* user_str = "default";
      if(uri_hp.field_set & (1 << UF_USERINFO)) {
        char* s = full_uri.mut_data() + uri_hp.field_data[UF_USERINFO].off;
        *(s + uri_hp.field_data[UF_USERINFO].len) = 0;
        user_str = s;
      }

      const char* host = "localhost";
      if(uri_hp.field_set & (1 << UF_HOST)) {
        char* s = full_uri.mut_data() + uri_hp.field_data[UF_HOST].off;
        *(s + uri_hp.field_data[UF_HOST].len) = 0;
        host = s;
      }

      uint16_t port = 6379;
      if(uri_hp.field_set & (1 << UF_PORT))
        port = uri_hp.port;

      const char* database = "";
      if(uri_hp.field_set & (1 << UF_PATH)) {
        char* s = full_uri.mut_data() + uri_hp.field_data[UF_PATH].off;
        *(s + uri_hp.field_data[UF_PATH].len) = 0;
        database = s + 1;  // skip initial slash
      }

      // Unmask the password which is sensitive data, so erasure shall be ensured.
      linear_buffer passwd_buf;
      passwd_buf.putn(this->m_password.data(), this->m_password.size() + 1);
      mask_string(passwd_buf.mut_data(), passwd_buf.size() - 1, nullptr, this->m_password_mask);

      const auto passwd_buf_guard = make_unique_handle(&passwd_buf,
          [&](...) {
            ::std::fill_n(static_cast<volatile char*>(passwd_buf.mut_begin()),
                          passwd_buf.size(), '\x9F');
          });

      // Try connecting to the server.
      if(!this->m_redis.reset(::redisConnect(host, port)))
        POSEIDON_THROW((
            "Could not connect to Redis server `$1`: ${errno:full}",
            "[`redisConnect()` failed]"),
            this->m_service_uri);

      if(this->m_redis->err != 0)
        POSEIDON_THROW((
            "Could not connect to Redis server `$1`: ERROR $2: $3",
            "[`redisConnect()` failed]"),
            this->m_service_uri, this->m_redis->err, this->m_redis->errstr);

      if(passwd_buf.size() > 1) {
        // `AUTH user password`
        if(!this->m_reply.reset(static_cast<::redisReply*>(::redisCommand(
                             this->m_redis, "AUTH %s %s", user_str, passwd_buf.data()))))
          POSEIDON_THROW((
              "Could not execute Redis command: ERROR $1: $2",
              "[`redisCommand()` failed]"),
              this->m_redis->err, this->m_redis->errstr);

        if(this->m_reply->type == REDIS_REPLY_ERROR)
          POSEIDON_THROW((
              "Failed to authenticate with Redis server: $1"),
              this->m_reply->str);
      }

      if(database[0]) {
        // `SELECT index`
        if(!this->m_reply.reset(static_cast<::redisReply*>(::redisCommand(
                                         this->m_redis, "SELECT %s", database))))
          POSEIDON_THROW((
              "Could not execute Redis command: ERROR $1: $2",
              "[`redisCommand()` failed]"),
              this->m_redis->err, this->m_redis->errstr);

        if(this->m_reply->type == REDIS_REPLY_ERROR)
          POSEIDON_THROW((
              "Could not set logical database: $1"),
              this->m_reply->str);
      }

      this->m_password.clear();
      this->m_password.shrink_to_fit();
      this->m_connected = true;
      POSEIDON_LOG_INFO(("Connected to Redis server `$1`"), this->m_service_uri);
    }

    // Discard the current reply.
    this->m_reply.reset();
    this->m_reset_clear = false;

    // Compose the argument and length vector.
    ::std::vector<uintptr_t> argv;
    argv.resize(ncmds * 2);

    for(size_t t = 0;  t != ncmds;  ++t) {
      argv.at(t) = reinterpret_cast<uintptr_t>(cmds[t].data());
      argv.at(ncmds + t) = cmds[t].size();
    }

    if(!this->m_reply.reset(static_cast<::redisReply*>(::redisCommandArgv(
                               this->m_redis, static_cast<int>(ncmds),
                               reinterpret_cast<const char**>(argv.data()),
                               reinterpret_cast<size_t*>(argv.data() + ncmds)))))
      POSEIDON_THROW((
          "Could not execute Redis command: ERROR $1: $2",
          "[`redisCommandArgv()` failed]"),
          this->m_redis->err, this->m_redis->errstr);

    if(this->m_reply->type == REDIS_REPLY_ERROR)
      POSEIDON_THROW((
          "Redis server replied with an error: $1"),
          this->m_reply->str);
  }

bool
Redis_Connection::
fetch_reply(Redis_Value& output)
  {
    output.clear();

    const auto unique_reply = move(this->m_reply);
    if(!unique_reply)
      return false;

    // Parse the reply and store the result into `output`.
    struct xFrame
      {
        Redis_Value* target;
        Redis_Array* psa;
        const ::redisReply* parent;
      };

    ::std::vector<xFrame> stack;
    Redis_Value* pval = &output;
    const ::redisReply* reply = unique_reply;

  do_pack_loop_:
    switch(reply->type)
      {
      case REDIS_REPLY_NIL:
        pval->clear();
        break;

      case REDIS_REPLY_INTEGER:
        pval->mut_integer() = reply->integer;
        break;

      case REDIS_REPLY_STRING:
        pval->mut_string().append(reply->str, reply->len);
        break;

      case REDIS_REPLY_ARRAY:
        if(reply->elements != 0) {
          // open
          auto& frm = stack.emplace_back();
          frm.target = pval;
          frm.psa = &(pval->mut_array());
          frm.parent = reply;
          frm.psa->reserve(static_cast<uint32_t>(reply->elements));
          pval = &(frm.psa->emplace_back());
          reply = reply->element[0];
          goto do_pack_loop_;
        }

        // empty
        pval->mut_array();
        break;
      }

    while(!stack.empty()) {
      auto& frm = stack.back();
      size_t t = frm.psa->size();
      if(t != frm.parent->elements) {
        // next
        pval = &(frm.psa->emplace_back());
        reply = frm.parent->element[t];
        goto do_pack_loop_;
      }

      // close
      pval = frm.target;
      stack.pop_back();
    }

    return true;
  }

}  // namespace poseidon
