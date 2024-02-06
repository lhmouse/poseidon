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
    cow_string m_conf_server;
    cow_string m_conf_user;
    cow_string m_conf_passwd;
    cow_string m_conf_db;
    uint16_t m_conf_port = 0;
    uint16_t m_conf_connection_pool_size = 0;
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
  };

}  // namespace poseidon
#endif
