// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongo_connector.hpp"
#include "../mongo/mongo_connection.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Pooled_Connection
  {
    uniptr<Mongo_Connection> conn;
    steady_time pooled_since = steady_time::max();
  };

void
do_mask_password(cow_string& password, uint32_t mask)
  {
    uint32_t mask_reg = mask;
    ::std::for_each(password.mut_begin(), password.mut_end(),
        [&](char& c) {
          c ^= static_cast<char>(mask_reg);
          mask_reg = mask_reg << 24 | mask_reg >> 8;
        });
  }

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Mongo_Connector, Pooled_Connection);

Mongo_Connector::
Mongo_Connector() noexcept
  {
  }

Mongo_Connector::
~Mongo_Connector()
  {
  }

void
Mongo_Connector::
reload(const Config_File& conf_file)
  {
    // Parse new configuration. Default ones are defined here.
    cow_string default_server = &"localhost";
    int64_t default_port = 27017;
    cow_string default_database;
    cow_string default_user = &"root";
    cow_string default_password;
    uint32_t password_mask = 0;
    int64_t connection_pool_size = 0;
    int64_t connection_idle_timeout = 60;

    // Read the server name from configuration. The Mongo client library is able
    // to perform DNS lookup as necessary, so this need not be an IP address.
    auto conf_value = conf_file.query("mongo", "default_server");
    if(conf_value.is_string())
      default_server = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mongo.default_server`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    // Read the port number from configuration. 0 is allowed as it has a special
    // meaning. 65535 is not allowed.
    conf_value = conf_file.query("mongo", "default_port");
    if(conf_value.is_integer())
      default_port = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mongo.default_port`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((default_port < 0) || (default_port > 65534))
      POSEIDON_THROW((
          "`mongo.default_port` value `$1` out of range",
          "[in configuration file '$2']"),
          default_port, conf_file.path());

    // Read the default database from configuration.
    conf_value = conf_file.query("mongo", "default_database");
    if(conf_value.is_string())
      default_database = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mongo.default_database`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    // Read the user name from configuration.
    conf_value = conf_file.query("mongo", "default_user");
    if(conf_value.is_string())
      default_user = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mongo.default_user`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    // Read the password from configuration. The password is not to be stored
    // as plaintext in memory.
    conf_value = conf_file.query("mongo", "default_password");
    if(conf_value.is_string())
      default_password = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mongo.default_password`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    password_mask = 0x80808080U | random_uint32();
    do_mask_password(default_password, password_mask);

    // Read the connection pool size from configuration.
    conf_value = conf_file.query("mongo", "connection_pool_size");
    if(conf_value.is_integer())
      connection_pool_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mongo.connection_pool_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((connection_pool_size < 0) || (connection_pool_size > 100))
      POSEIDON_THROW((
          "`mongo.connection_pool_size` value `$1` out of range",
          "[in configuration file '$2']"),
          connection_pool_size, conf_file.path());

    // Read the idle timeout from configuration, as the number of seconds.
    conf_value = conf_file.query("mongo", "connection_idle_timeout");
    if(conf_value.is_integer())
      connection_idle_timeout = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mongo.connection_idle_timeout`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((connection_idle_timeout < 0) || (connection_idle_timeout > 8640000))
      POSEIDON_THROW((
          "`mongo.connection_idle_timeout` value `$1` out of range",
          "[in configuration file '$2']"),
          connection_idle_timeout, conf_file.path());

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_default_server.swap(default_server);
    this->m_conf_default_port = static_cast<uint16_t>(default_port);
    this->m_conf_default_user.swap(default_user);
    this->m_conf_default_password.swap(default_password);
    this->m_conf_password_mask = password_mask;
    this->m_conf_default_database.swap(default_database);
    this->m_conf_connection_pool_size = static_cast<uint16_t>(connection_pool_size);
    this->m_conf_connection_idle_timeout = static_cast<seconds>(connection_idle_timeout);
  }

uniptr<Mongo_Connection>
Mongo_Connector::
allocate_connection(cow_stringR server, uint16_t port, cow_stringR db, cow_stringR user, cow_stringR passwd)
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const uint32_t pool_size = this->m_conf_connection_pool_size;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Iterate over the pool, looking for a matching connection with minimum
    // timestamp.
    lock.lock(this->m_pool_mutex);
    Pooled_Connection* target = nullptr;

    const steady_time expired_since = steady_clock::now() - idle_timeout;
    this->m_pool.resize(pool_size);

    for(auto& elem : this->m_pool)
      if(elem.pooled_since < expired_since) {
        // expired
        elem.conn.reset();
        elem.pooled_since = steady_time::max();
      }
      else if(elem.conn && (elem.conn->m_server == server) && (elem.conn->m_port == port)
                        && (elem.conn->m_db == db) && (elem.conn->m_user == user)
                        && (!target || (elem.pooled_since < target->pooled_since)))
        target = &elem;

    if(!target) {
      lock.unlock();

      // Create a new connection.
      return new_uni<Mongo_Connection>(server, port, db, user, passwd);
    }

    // Pop this connection from the pool.
    auto conn = move(target->conn);
    target->pooled_since = steady_time::max();
    return conn;
  }

uniptr<Mongo_Connection>
Mongo_Connector::
allocate_default_connection()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const cow_string server = this->m_conf_default_server;
    const uint16_t port = this->m_conf_default_port;
    const cow_string db = this->m_conf_default_database;
    const cow_string user = this->m_conf_default_user;
    const cow_string xpasswd = this->m_conf_default_password;
    const uint32_t passwd_mask = this->m_conf_password_mask;
    const uint32_t pool_size = this->m_conf_connection_pool_size;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Iterate over the pool, looking for a matching connection with minimum
    // timestamp.
    lock.lock(this->m_pool_mutex);
    Pooled_Connection* target = nullptr;

    const steady_time expired_since = steady_clock::now() - idle_timeout;
    this->m_pool.resize(pool_size);

    for(auto& elem : this->m_pool)
      if(elem.pooled_since < expired_since) {
        // expired
        elem.conn.reset();
        elem.pooled_since = steady_time::max();
      }
      else if(elem.conn && (elem.conn->m_server == server) && (elem.conn->m_port == port)
                        && (elem.conn->m_db == db) && (elem.conn->m_user == user)
                        && (!target || (elem.pooled_since < target->pooled_since)))
        target = &elem;

    if(!target) {
      lock.unlock();

      // Create a new connection.
      cow_string passwd;
      passwd.assign(xpasswd.begin(), xpasswd.end());
      do_mask_password(passwd, passwd_mask);
      return new_uni<Mongo_Connection>(server, port, db, user, passwd);
    }

    // Pop this connection from the pool.
    auto conn = move(target->conn);
    target->pooled_since = steady_time::max();
    return conn;
  }

bool
Mongo_Connector::
pool_connection(uniptr<Mongo_Connection>&& conn) noexcept
  {
    if(!conn)
      return false;

    if(!conn->m_reset_clear) {
      // If `.reset()` has not been called or has failed, the connection cannot
      // be reused safely, so ignore the request.
      POSEIDON_LOG_ERROR(("MongoDB connection not reset properly"));
      return false;
    }

    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Iterate over the pool, looking for a matching connection with minimum
    // timestamp.
    lock.lock(this->m_pool_mutex);
    Pooled_Connection* target = nullptr;

    for(auto& elem : this->m_pool)
      if(!elem.conn) {
        // empty
        target = &elem;
        break;
      }
      else if(!target || (elem.pooled_since < target->pooled_since))
        target = &elem;

    if(!target)
      return false;

    // Move the connection into this element.
    target->conn.swap(conn);
    target->pooled_since = steady_clock::now();
    lock.unlock();

    // Free the replaced connection before returning, if any.
    conn.reset();
    return true;
  }

}  // namespace poseidon