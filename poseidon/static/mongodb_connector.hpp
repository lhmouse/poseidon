// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_MONGODB_CONNECTOR_
#define POSEIDON_STATIC_MONGODB_CONNECTOR_

#include "../fwd.hpp"
#include "../third/mongodb_fwd.hpp"
namespace poseidon {

class MongoDB_Connector
  {
  private:
    mutable plain_mutex m_conf_mutex;
    cow_string m_conf_default_server;
    cow_string m_conf_default_user;
    cow_string m_conf_default_password;
    cow_string m_conf_default_database;
    uint16_t m_conf_default_port = 0;
    uint16_t m_conf_connection_pool_size = 0;
    uint32_t m_conf_password_mask = 0;
    seconds m_conf_connection_idle_timeout = 0s;

    mutable plain_mutex m_pool_mutex;
    struct X_Pooled_Connection;
    vector<X_Pooled_Connection> m_pool;

  public:
    // Constructs an empty connector.
    MongoDB_Connector() noexcept;

  public:
    MongoDB_Connector(const MongoDB_Connector&) = delete;
    MongoDB_Connector& operator=(const MongoDB_Connector&) & = delete;
    ~MongoDB_Connector();

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& conf_file);

    // Allocates a connection to `server:port` as `user`, and sets the default
    // database to `db`. If a matching idle connection exists in the pool, it is
    // returned; otherwise a new connection is created.
    uniptr<MongoDB_Connection>
    allocate_connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd, cow_stringR db);

    // Allocates a connection using arguments from 'main.conf'. This function
    // is otherwise the same as `allocate_connection_explicit()`.
    uniptr<MongoDB_Connection>
    allocate_default_connection();

    // Puts a connection back into the pool. It is required to `.reset()` a
    // connection before putting it back. Resetting a connection is a blocking
    // operation that we can't afford. Hence, if the connection has not been
    // reset, a warning is printed, and the request is ignored, and `false` is
    // returned.
    bool
    pool_connection(uniptr<MongoDB_Connection>&& conn) noexcept;
  };

}  // namespace poseidon
#endif
