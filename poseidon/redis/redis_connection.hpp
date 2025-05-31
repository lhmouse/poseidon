// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

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

    cow_string m_service_uri;
    cow_string m_password;
    bool m_connected;
    bool m_reset_clear;
    char m_reserved_1;
    char m_reserved_2;
    steady_time m_time_pooled;

    uniptr_redisContext m_redis;
    uniptr_redisReply m_reply;

  public:
    // Sets connection parameters. This function does not attempt to connect
    // to the server, and is not blocking.
    Redis_Connection(cow_stringR service_uri, cow_stringR password);

  public:
    Redis_Connection(const Redis_Connection&) = delete;
    Redis_Connection& operator=(const Redis_Connection&) & = delete;
    ~Redis_Connection();

    // Gets the URI from the constructor.
    cow_stringR
    service_uri() const noexcept
      { return this->m_service_uri;  }

    // Resets the connection so it can be reused by another thread. This is a
    // blocking functions. DO NOT ATTEMPT TO REUSE THE CONNECTION IF THIS
    // FUNCTION RETURNS `false`.
    // Returns whether the connection may be safely reused.
    bool
    reset() noexcept;

    // Executes a command as an array of strings.
    void
    execute(const cow_vector<cow_string>& cmd);

    // Gets the reply of the previous command. `output` is cleared before any
    // operation. If the reply has been stored into `output`, `true` is returned.
    // If there is no more reply, `false` is returned.
    bool
    fetch_reply(Redis_Value& output);
  };

}  // namespace poseidon
#endif
