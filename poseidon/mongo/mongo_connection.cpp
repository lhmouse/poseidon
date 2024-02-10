// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_connection.hpp"
//#include "mongo_value.hpp"
#include "../utils.hpp"
namespace poseidon {

Mongo_Connection::
Mongo_Connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd, cow_stringR db)
  :
    m_server(server), m_user(user), m_passwd(passwd), m_db(db), m_port(port),
    m_connected(false), m_reset_clear(true)
  {
  }

Mongo_Connection::
~Mongo_Connection()
  {
  }

}  // namespace poseidon
