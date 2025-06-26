// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_MYSQL_CONNECTOR_
#define POSEIDON_STATIC_MYSQL_CONNECTOR_

#include "../fwd.hpp"
#include "../details/mysql_fwd.hpp"
namespace poseidon {

class MySQL_Connector
  {
  private:
    mutable plain_mutex m_conf_mutex;
    cow_string m_conf_default_service_uri;
    cow_string m_conf_default_password;
    cow_string m_conf_secondary_service_uri;
    cow_string m_conf_secondary_password;
    cow_string m_conf_tertiary_service_uri;
    cow_string m_conf_tertiary_password;
    uint32_t m_conf_connection_pool_size = 0;
    seconds m_conf_connection_idle_timeout = 0s;

    mutable plain_mutex m_pool_mutex;
    struct X_Pooled_Connection;
    cow_vector<X_Pooled_Connection> m_pool;

  public:
    // Constructs an empty connector.
    MySQL_Connector() noexcept;

  private:
    uniptr<MySQL_Connection>
    do_get_pooled_connection_opt(seconds idle_timeout, const cow_string& service_uri);

  public:
    MySQL_Connector(const MySQL_Connector&) = delete;
    MySQL_Connector& operator=(const MySQL_Connector&) & = delete;
    ~MySQL_Connector();

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& conf_file);

    // Allocates a connection to `user@server:port/database`. If a matching
    // idle connection exists in the pool, it is returned; otherwise a new
    // connection is created.
    uniptr<MySQL_Connection>
    allocate_connection(const cow_string& service_uri, const cow_string& password);

    // Allocates a connection using arguments from 'main.conf'. This function
    // is otherwise the same as `allocate_connection()`.
    uniptr<MySQL_Connection>
    allocate_default_connection();

    // Allocates a connection using secondary arguments from 'main.conf'. If
    // no secondary configuration exists, the default configuration is used.
    // This function is otherwise the same as `allocate_connection()`.
    uniptr<MySQL_Connection>
    allocate_secondary_connection();

    // Allocates a connection using tertiary arguments from 'main.conf'. If
    // no tertiary configuration exists, the secondary configuration is used.
    // This function is otherwise the same as `allocate_connection()`.
    uniptr<MySQL_Connection>
    allocate_tertiary_connection();

    // Puts a connection back into the pool. It is required to `.reset()` a
    // connection before putting it back. Resetting a connection is a blocking
    // operation that we can't afford. Hence, if the connection has not been
    // reset, a warning is printed, and the request is ignored, and `false` is
    // returned.
    bool
    pool_connection(uniptr<MySQL_Connection>&& conn);
  };

}  // namespace poseidon
#endif
