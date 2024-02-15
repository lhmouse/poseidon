// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_REDIS_REDIS_CONNECTION_
#define POSEIDON_REDIS_REDIS_CONNECTION_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../third/redis_fwd.hpp"
namespace poseidon {

class Redis_Connection
  {
  private:
    friend class Redis_Connector;

    cow_string m_server;
    cow_string m_user;
    uint16_t m_port;

    bool m_connected;
    bool m_reset_clear;
    uint32_t m_passwd_mask;
    cow_string m_passwd;
    steady_time m_time_pooled;

    uniptr_redisContext m_redis;
    uniptr_redisReply m_reply;

  public:
    // Sets connection parameters. This function does not attempt to connect
    // to the server, and is not blocking.
    Redis_Connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd);

  public:
    Redis_Connection(const Redis_Connection&) = delete;
    Redis_Connection& operator=(const Redis_Connection&) & = delete;
    ~Redis_Connection();

    // Get connection parameters.
    cow_stringR
    server() const noexcept
      { return this->m_server;  }

    uint16_t
    port() const noexcept
      { return this->m_port;  }

    cow_stringR
    user() const noexcept
      { return this->m_user;  }

    // Resets the connection so it can be reused by another thread. This is a
    // blocking functions. DO NOT ATTEMPT TO REUSE THE CONNECTION IF THIS
    // FUNCTION RETURNS `false`.
    // Returns whether the connection may be safely reused.
    bool
    reset() noexcept;

    // Executes a command as an array of strings.
    void
    execute(const cow_string* cmds, size_t ncmds);

    // Gets the reply of the previous command. `output` is cleared before any
    // operation. If the reply has been stored into `output`, `true` is returned.
    // If there is no more reply, `false` is returned.
    bool
    fetch_reply(Redis_Value& output);
  };

}  // namespace poseidon
#endif
