// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MONGO_MONGO_CONNECTION_
#define POSEIDON_MONGO_MONGO_CONNECTION_

#include "../fwd.hpp"
#include "enums.hpp"
#include "../third/mongo_fwd.hpp"
#include "enums.hpp"
namespace poseidon {

class Mongo_Connection
  {
  private:
    friend class Mongo_Connector;

    cow_string m_server;
    cow_string m_user;
    cow_string m_passwd;
    cow_string m_db;
    uint16_t m_port;
    bool m_connected;
    bool m_reset_clear;

  public:
    // Sets connection parameters. This function does not attempt to connect
    // to the server, and is not blocking.
    Mongo_Connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd, cow_stringR db);

  public:
    Mongo_Connection(const Mongo_Connection&) = delete;
    Mongo_Connection& operator=(const Mongo_Connection&) & = delete;
    ~Mongo_Connection();

    // Get connection parameters.
    cow_stringR
    server() const noexcept
      { return this->m_server;  }

    cow_stringR
    user() const noexcept
      { return this->m_user;  }

    cow_stringR
    db() const noexcept
      { return this->m_db;  }

    uint16_t
    port() const noexcept
      { return this->m_port;  }
  };

}  // namespace poseidon
#endif
