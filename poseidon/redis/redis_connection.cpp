// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

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
      linear_buffer uri_buf;
      uri_buf.putn("t://", 4);
      uri_buf.putn(this->m_service_uri.data(), this->m_service_uri.size() + 1);

      ::http_parser_url uri_hp;
      ::http_parser_url_init(&uri_hp);
      if(::http_parser_parse_url(uri_buf.mut_data(), uri_buf.size() - 1, false, &uri_hp) != 0)
        POSEIDON_THROW((
            "Invalid Redis service URI `$1`",
            "[`http_parser_parse_url()` failed]"),
            this->m_service_uri);

      if(uri_hp.field_set & (1 << UF_PATH | 1 << UF_QUERY))
        POSEIDON_THROW((
            "Path or options not allowed in Redis service URI `$1`"),
            this->m_service_uri);

      const char* user_str = "default";
      size_t user_len = 7;
      if(uri_hp.field_set & (1 << UF_USERINFO)) {
        char* s = uri_buf.mut_data() + uri_hp.field_data[UF_USERINFO].off;
        *(s + uri_hp.field_data[UF_USERINFO].len) = 0;
        user_str = s;
        user_len = uri_hp.field_data[UF_USERINFO].len;
      }

      const char* host = "localhost";
      if(uri_hp.field_set & (1 << UF_HOST)) {
        char* s = uri_buf.mut_data() + uri_hp.field_data[UF_HOST].off;
        *(s + uri_hp.field_data[UF_HOST].len) = 0;
        host = s;
      }

      uint16_t port = 6379;
      if(uri_hp.field_set & (1 << UF_PORT))
        port = uri_hp.port;

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
        const char* argv[] = { "AUTH", user_str, passwd_buf.data() };
        size_t lenv[] = { 4, user_len, passwd_buf.size() - 1 };
        if(!this->m_reply.reset(static_cast<::redisReply*>(::redisCommandArgv(
                                   this->m_redis, 3, argv, lenv))))
          POSEIDON_THROW((
              "Could not execute Redis command: ERROR $1: $2",
              "[`redisCommandArgv()` failed]"),
              this->m_redis->err, this->m_redis->errstr);

        if(this->m_reply->type == REDIS_REPLY_ERROR)
          POSEIDON_THROW((
              "Failed to authenticate with Redis server: $1"),
              this->m_reply->str);
      }

      // Get the local address.
      ::sockaddr_storage ss;
      ::socklen_t sslen = sizeof(ss);
      if(::getsockname(this->m_redis->fd, (::sockaddr*) &ss, &sslen) != 0)
        POSEIDON_THROW((
            "Could not get local address: ${errno:full}",
            "[`getsockname()` failed]"));

      switch(ss.ss_family)
        {
        case AF_INET:
          // IPv4
          ::memcpy(this->m_local_addr.mut_data(), ipv4_unspecified.data(), 12);
          ::memcpy(this->m_local_addr.mut_data() + 12, &(((::sockaddr_in*) &ss)->sin_addr), 4);
          this->m_local_addr.set_port(ROCKET_BETOH16(((::sockaddr_in*) &ss)->sin_port));
          break;

        case AF_INET6:
          // IPv6
          ::memcpy(this->m_local_addr.mut_data(), &(((::sockaddr_in6*) &ss)->sin6_addr), 16);
          this->m_local_addr.set_port(ROCKET_BETOH16(((::sockaddr_in6*) &ss)->sin6_port));
          break;

        default:
          POSEIDON_THROW(("Socket address family `$1` not supported"), ss.ss_family);
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
    vector<uintptr_t> argv;
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

    vector<xFrame> stack;
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
