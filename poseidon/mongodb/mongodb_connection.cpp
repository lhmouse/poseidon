// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongodb_connection.hpp"
//#include "mongodb_value.hpp"
#include "../utils.hpp"
namespace poseidon {

MongoDB_Connection::
MongoDB_Connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd, cow_stringR db)
  :
    m_server(server), m_user(user), m_passwd(passwd), m_db(db), m_port(port),
    m_connected(false), m_reset_clear(true)
  {
  }

MongoDB_Connection::
~MongoDB_Connection()
  {
  }

}  // namespace poseidon
