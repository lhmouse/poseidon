// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_connection.hpp"
#include "redis_value.hpp"
#include "../utils.hpp"
namespace poseidon {

Redis_Connection::
Redis_Connection(const cow_string& service_uri, const cow_string& password)
  {
    this->m_service_uri = service_uri;
    this->m_password = password;
    this->m_connected = false;
    this->m_reset_clear = true;
  }

Redis_Connection::
~Redis_Connection()
  {
  }

bool
Redis_Connection::
reset()
  noexcept
  {
    // Discard the current reply.
    this->m_reply.reset();

    // Check whether an error has occurred.
    bool error = this->m_connected && (this->m_redis->err != 0);
    this->m_reset_clear = !error;
    return !error;
  }

void
Redis_Connection::
execute(const cow_vector<cow_string>& cmd)
  {
    if(cmd.empty())
      POSEIDON_THROW(("Empty Redis command"));

    if(!this->m_connected) {
      // Parse the service URI in a hacky way.
      cow_string user = &"default";
      cow_string host = &"localhost";
      uint16_t port = 6379;
      cow_string database = &"0";

      cow_string uri = this->m_service_uri;
      size_t t = uri.find_of("@/");
      if((t != cow_string::npos) && (uri[t] == '@')) {
        user.assign(uri, 0, t);
        uri.erase(0, t + 1);
      }

      Network_Reference ref;
      if(parse_network_reference(ref, uri) != uri.size())
        POSEIDON_THROW((
            "Invalid Redis service URI `$1`",
            "[`parse_network_reference()` failed]"),
            this->m_service_uri);

      if(ref.host.n != 0)
        host.assign(ref.host.p, ref.host.n);

      if(ref.port.n != 0)
        port = ref.port_num;

      if(ref.path.n != 0)
        database.assign(ref.path.p + 1, ref.path.n - 1);

      // Try connecting to the server.
      if(!this->m_redis.reset(::redisConnect(host.c_str(), port)))
        POSEIDON_THROW((
            "Could not connect to Redis server `$1`: ${errno:full}",
            "[`redisConnect()` failed]"),
            this->m_service_uri);

      if(this->m_redis->err != 0)
        POSEIDON_THROW((
            "Could not connect to Redis server `$1`: ERROR $2: $3",
            "[`redisConnect()` failed]"),
            this->m_service_uri, this->m_redis->err, this->m_redis->errstr);

      if(this->m_password.size() != 0) {
        // `AUTH user password`
        if(!this->m_reply.reset(static_cast<::redisReply*>(::redisCommand(
                                    this->m_redis, "AUTH %s %s", user.c_str(),
                                    this->m_password.c_str()))))
          POSEIDON_THROW((
              "Could not execute Redis command: ERROR $1: $2",
              "[`redisCommand()` failed]"),
              this->m_redis->err, this->m_redis->errstr);

        if(this->m_reply->type == REDIS_REPLY_ERROR)
          POSEIDON_THROW((
              "Failed to authenticate with Redis server: $1"),
              this->m_reply->str);
      }

      if(database.size() != 0) {
        // `SELECT index`
        if(!this->m_reply.reset(static_cast<::redisReply*>(::redisCommand(
                                    this->m_redis, "SELECT %s", database.c_str()))))
          POSEIDON_THROW((
              "Could not execute Redis command: ERROR $1: $2",
              "[`redisCommand()` failed]"),
              this->m_redis->err, this->m_redis->errstr);

        if(this->m_reply->type == REDIS_REPLY_ERROR)
          POSEIDON_THROW((
              "Could not set logical database: $1"),
              this->m_reply->str);
      }

      cow_string().swap(this->m_password);
      this->m_connected = true;
      POSEIDON_LOG_INFO(("Connected to Redis server `$1`"), this->m_service_uri);
    }

    // Discard the current reply.
    this->m_reply.reset();
    this->m_reset_clear = false;

    // Compose the argument and length vector.
    ::std::vector<uintptr_t> argv;
    argv.resize(cmd.size() * 2);

    for(size_t t = 0;  t != cmd.size();  ++t) {
      argv.at(t) = reinterpret_cast<uintptr_t>(cmd.at(t).data());
      argv.at(cmd.size() + t) = cmd.at(t).size();
    }

    if(!this->m_reply.reset(static_cast<::redisReply*>(::redisCommandArgv(
                                this->m_redis, static_cast<int>(cmd.size()),
                                reinterpret_cast<const char**>(argv.data()),
                                reinterpret_cast<size_t*>(argv.data() + cmd.size())))))
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
fetch_reply(cow_string& status, Redis_Value& value)
  {
    status.clear();
    value.clear();

    const auto unique_reply = move(this->m_reply);
    if(!unique_reply)
      return false;

    // Parse the reply and store the result into `value`.
    struct xFrame
      {
        Redis_Value* target;
        Redis_Array* psa;
        const ::redisReply* parent;
      };

    ::std::vector<xFrame> stack;
    Redis_Value* pval = &value;
    const ::redisReply* reply = unique_reply;

    if(reply->type == REDIS_REPLY_STATUS) {
      // This is not a value.
      status.append(reply->str, reply->len);
      return true;
    }

  do_pack_loop_:
    switch(reply->type)
      {
      case REDIS_REPLY_NIL:
        pval->clear();
        break;

      case REDIS_REPLY_INTEGER:
        pval->open_integer() = reply->integer;
        break;

      case REDIS_REPLY_STRING:
        pval->open_string().append(reply->str, reply->len);
        break;

      case REDIS_REPLY_ARRAY:
        if(reply->elements != 0) {
          // open
          auto& frm = stack.emplace_back();
          frm.target = pval;
          frm.psa = &(pval->open_array());
          frm.parent = reply;
          frm.psa->reserve(static_cast<uint32_t>(reply->elements));
          pval = &(frm.psa->emplace_back());
          reply = reply->element[0];
          goto do_pack_loop_;
        }

        // empty
        pval->open_array();
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
