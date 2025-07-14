// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_MONGO_CONNECTOR_
#define POSEIDON_STATIC_MONGO_CONNECTOR_

#include "../fwd.hpp"
#include "../details/mongo_fwd.hpp"
namespace poseidon {

class Mongo_Connector
  {
  private:
    mutable plain_mutex m_conf_mutex;
    cow_string m_conf_default_service_uri;
    cow_string m_conf_default_password;
    uint32_t m_conf_connection_pool_size = 0;
    seconds m_conf_connection_idle_timeout = 0s;

    mutable plain_mutex m_pool_mutex;
    struct X_Pooled_Connection;
    cow_vector<X_Pooled_Connection> m_pool;

  public:
    // Constructs an empty connector.
    Mongo_Connector()
      noexcept;

  private:
    uniptr<Mongo_Connection>
    do_get_pooled_connection_opt(seconds idle_timeout, const cow_string& service_uri);

  public:
    Mongo_Connector(const Mongo_Connector&) = delete;
    Mongo_Connector& operator=(const Mongo_Connector&) & = delete;
    ~Mongo_Connector();

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& conf_file);

    // Allocates a connection to `user@server:port/database`. If a matching
    // idle connection exists in the pool, it is returned; otherwise a new
    // connection is created.
    uniptr<Mongo_Connection>
    allocate_connection(const cow_string& service_uri, const cow_string& password);

    // Allocates a connection using arguments from 'main.conf'. This function
    // is otherwise the same as `allocate_connection()`.
    uniptr<Mongo_Connection>
    allocate_default_connection();

    // Puts a connection back into the pool. It is required to `.reset()` a
    // connection before putting it back. Resetting a connection is a blocking
    // operation that we can't afford. Hence, if the connection has not been
    // reset, a warning is printed, and the request is ignored, and `false` is
    // returned.
    bool
    pool_connection(uniptr<Mongo_Connection>&& conn);
  };

}  // namespace poseidon
#endif
