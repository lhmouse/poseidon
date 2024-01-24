// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_MYSQL_CONNECTOR_
#define POSEIDON_STATIC_MYSQL_CONNECTOR_

#include "../fwd.hpp"
#include "../third/mysql_fwd.hpp"
namespace poseidon {

class MySQL_Connector
  {
  private:
    unique_posix_fd m_epoll;

    mutable plain_mutex m_conf_mutex;

    mutable plain_mutex m_pool_mutex;

  public:
    // Constructs an empty connector.
    explicit
    MySQL_Connector();

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(MySQL_Connector);

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& conf_file);
  };

}  // namespace poseidon
#endif
