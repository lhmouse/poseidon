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

    if(!this->m_reply)
      return false;

    // Parse the reply and store the result into `output`.
    struct rb_context
      {
        Redis_Value* output;
        ::redisReply* reply;
        size_t pos = 0;
      };

    vector<rb_context> rb_stack;
    rb_stack.emplace_back();
    rb_stack.back().output = &output;
    rb_stack.back().reply = this->m_reply;

    while(!rb_stack.empty()) {
      auto& rb_top = rb_stack.back();

      // Unpack a reply.
      switch(rb_top.reply->type)
        {
        case REDIS_REPLY_NIL:
          rb_top.output->clear();
          break;

        case REDIS_REPLY_INTEGER:
          rb_top.output->mut_integer() = rb_top.reply->integer;
          break;

        case REDIS_REPLY_STRING:
          rb_top.output->mut_string().assign(rb_top.reply->str, rb_top.reply->len);
          break;

        case REDIS_REPLY_ARRAY:
          {
            auto& output_arr = rb_top.output->mut_array();
            output_arr.resize(rb_top.reply->elements);

            if(rb_top.pos < rb_top.reply->elements) {
              // Descend into this array.
              rb_context rb_next;
              rb_next.output = &(output_arr.mut(rb_top.pos));
              rb_next.reply = rb_top.reply->element[rb_top.pos];
              rb_top.pos ++;
              rb_stack.push_back(rb_next);
              continue;
            }
          }
          break;
        }

      rb_stack.pop_back();
    }

    this->m_reply.reset();

    // Return the result into `output`.
    return true;
  }

}  // namespace poseidon
