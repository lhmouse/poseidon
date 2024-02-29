// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_connection.hpp"
#include "redis_value.hpp"
#include "../utils.hpp"
namespace poseidon {

Redis_Connection::
Redis_Connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd)
  {
    this->m_server = server;
    this->m_port = port;
    this->m_user = user;

    server.safe_c_str();
    user.safe_c_str();
    passwd.safe_c_str();

    this->m_reset_clear = true;
    this->m_passwd_mask = 0x80000000U | random_uint32();
    this->m_passwd = passwd;
    mask_string(this->m_passwd.mut_data(), this->m_passwd.size(), nullptr, this->m_passwd_mask);
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
      // Try connecting to the server now.
      mask_string(this->m_passwd.mut_data(), this->m_passwd.size(), nullptr, this->m_passwd_mask);

      this->m_reply.reset();
      this->m_redis.reset(::redisConnect(this->m_server.c_str(), this->m_port));

      if(this->m_redis && (this->m_redis->err == 0) && !this->m_passwd.empty()) {
        // Authenticate.
        static_vector<const char*, 3> argv = { "AUTH", this->m_passwd.data() };
        static_vector<size_t, 3> lenv = { 4, this->m_passwd.size() };

        if(!this->m_user.empty()) {
          // `AUTH [user] pass`
          argv.insert(1, this->m_user.data());
          lenv.insert(1, this->m_user.size());
        }

        this->m_reply.reset(static_cast<::redisReply*>(::redisCommandArgv(this->m_redis,
                  static_cast<int>(argv.size()), argv.mut_data(), lenv.mut_data())));
      }

      this->m_passwd_mask = 0x80000000U | random_uint32();
      mask_string(this->m_passwd.mut_data(), this->m_passwd.size(), nullptr, this->m_passwd_mask);

      if(!this->m_redis)
        POSEIDON_THROW((
            "Could not allocate Redis context",
            "[`redisConnect()` failed]"));

      if(this->m_redis->err != 0)
        POSEIDON_THROW((
            "Could not connect to Redis server: ERROR $1: $2",
            "[`redisConnect()` failed]"),
            this->m_redis->err, this->m_redis->errstr);

      if(this->m_reply && (this->m_reply->type == REDIS_REPLY_ERROR))
        POSEIDON_THROW((
            "Failed to authenticate with Redis server: $1"),
            this->m_reply->str);

      cow_string().swap(this->m_passwd);
      this->m_connected = true;

      POSEIDON_LOG_INFO((
          "Connected to Redis server `$3@$1:$2`"),
          this->m_server, this->m_port, this->m_user);
    }

    // Discard the current reply.
    this->m_reply.reset();
    this->m_reset_clear = false;

    // Compose the argument and length vector.
    vector<uintptr_t> argv;
    argv.resize(ncmds * 2);

    for(size_t t = 0;  t != ncmds;  ++t) {
      argv.at(t) = reinterpret_cast<uintptr_t>(cmds[t].data());
      argv.at(ncmds + t) = cmds[t].size();
    }

    this->m_reply.reset(static_cast<::redisReply*>(::redisCommandArgv(this->m_redis,
                          static_cast<int>(ncmds), reinterpret_cast<const char**>(argv.data()),
                          reinterpret_cast<size_t*>(argv.data() + ncmds))));
    if(!this->m_reply)
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
        Redis_Array vsa;
        const ::redisReply* parent;
      };

    vector<xFrame> stack;
    const ::redisReply* reply = unique_reply;

  do_pack_loop_:
    switch(reply->type)
      {
      case REDIS_REPLY_NIL:
        output.clear();
        break;

      case REDIS_REPLY_INTEGER:
        output.mut_integer() = reply->integer;
        break;

      case REDIS_REPLY_STRING:
        output.mut_string().assign(reply->str, reply->len);
        break;

      case REDIS_REPLY_ARRAY:
        if(reply->elements != 0) {
          // open
          auto& frm = stack.emplace_back();
          frm.vsa.reserve(reply->elements);
          frm.parent = reply;
          reply = reply->element[0];
          goto do_pack_loop_;
        }

        // empty
        output.mut_array().clear();
        break;
      }

    while(!stack.empty()) {
      auto& frm = stack.back();
      frm.vsa.emplace_back().swap(output);

      size_t n = frm.vsa.size();
      if(n != frm.parent->elements) {
        // next
        ROCKET_ASSERT(n < frm.parent->elements);
        reply = frm.parent->element[n];
        goto do_pack_loop_;
      }

      ROCKET_ASSERT(output.type() == redis_value_null);
      output = move(frm.vsa);

      // close
      stack.pop_back();
    }

    return true;
  }

}  // namespace poseidon
