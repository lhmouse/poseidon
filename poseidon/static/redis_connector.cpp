// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "redis_connector.hpp"
#include "../redis/redis_connection.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <rocket/once_flag.hpp>
namespace poseidon {

Redis_Connector::
Redis_Connector() noexcept
  {
  }

Redis_Connector::
~Redis_Connector()
  {
  }

void
Redis_Connector::
reload(const Config_File& conf_file)
  {
    // Parse new configuration. Default ones are defined here.
    cow_string default_server = &"localhost";
    int64_t default_port = 6379;
    cow_string default_user = &"root";
    cow_string default_password;
    uint32_t password_mask = 0;
    int64_t connection_pool_size = 0;
    int64_t connection_idle_timeout = 60;

    // Read the server name from configuration. The hiredis library is able to
    // perform DNS lookup as necessary, so this need not be an IP address.
    auto conf_value = conf_file.query("redis", "default_server");
    if(conf_value.is_string())
      default_server = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `redis.default_server`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    // Read the port number from configuration. 0 is allowed as it has a special
    // meaning. 65535 is not allowed.
    conf_value = conf_file.query("redis", "default_port");
    if(conf_value.is_integer())
      default_port = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `redis.default_port`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((default_port < 0) || (default_port > 65534))
      POSEIDON_THROW((
          "`redis.default_port` value `$1` out of range",
          "[in configuration file '$2']"),
          default_port, conf_file.path());

    // Read the user name from configuration.
    conf_value = conf_file.query("redis", "default_user");
    if(conf_value.is_string())
      default_user = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `redis.default_user`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    // Read the password from configuration. The password is not to be stored
    // as plaintext in memory.
    conf_value = conf_file.query("redis", "default_password");
    if(conf_value.is_string())
      default_password = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `redis.default_password`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    password_mask = 0x80808080U | random_uint32();
    mask_string(default_password.mut_data(), default_password.size(), nullptr, password_mask);

    // Read the connection pool size from configuration.
    conf_value = conf_file.query("redis", "connection_pool_size");
    if(conf_value.is_integer())
      connection_pool_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `redis.connection_pool_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((connection_pool_size < 0) || (connection_pool_size > 100))
      POSEIDON_THROW((
          "`redis.connection_pool_size` value `$1` out of range",
          "[in configuration file '$2']"),
          connection_pool_size, conf_file.path());

    // Read the idle timeout from configuration, as the number of seconds.
    conf_value = conf_file.query("redis", "connection_idle_timeout");
    if(conf_value.is_integer())
      connection_idle_timeout = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `redis.connection_idle_timeout`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((connection_idle_timeout < 0) || (connection_idle_timeout > 8640000))
      POSEIDON_THROW((
          "`redis.connection_idle_timeout` value `$1` out of range",
          "[in configuration file '$2']"),
          connection_idle_timeout, conf_file.path());

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_default_server.swap(default_server);
    this->m_conf_default_port = static_cast<uint16_t>(default_port);
    this->m_conf_default_user.swap(default_user);
    this->m_conf_default_password.swap(default_password);
    this->m_conf_password_mask = password_mask;
    this->m_conf_connection_pool_size = static_cast<uint16_t>(connection_pool_size);
    this->m_conf_connection_idle_timeout = static_cast<seconds>(connection_idle_timeout);
  }

uniptr<Redis_Connection>
Redis_Connector::
allocate_connection(cow_stringR server, uint16_t port, cow_stringR user, cow_stringR passwd)
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const uint32_t pool_size = this->m_conf_connection_pool_size;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Iterate over the pool, looking for a matching connection with minimum
    // timestamp.
    lock.lock(this->m_pool_mutex);
    this->m_pool.resize(pool_size);
    uniptr<Redis_Connection>* use_slot = nullptr;
    const steady_time now = steady_clock::now();

    for(auto& slot : this->m_pool) {
      if(slot && (now - slot->m_time_pooled > idle_timeout))
        slot.reset();

      if(slot && (!use_slot || (slot->m_time_pooled < (*use_slot)->m_time_pooled))
              && (slot->m_server == server) && (slot->m_port == port)
              && (slot->m_user == user))
        use_slot = &slot;
    }

    if(use_slot)
      return move(*use_slot);

    lock.unlock();

    // Create a new connection.
    return new_uni<Redis_Connection>(server, port, user, passwd);
  }

uniptr<Redis_Connection>
Redis_Connector::
allocate_default_connection()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const cow_string server = this->m_conf_default_server;
    const uint16_t port = this->m_conf_default_port;
    const cow_string user = this->m_conf_default_user;
    const cow_string default_password = this->m_conf_default_password;
    const uint32_t password_mask = this->m_conf_password_mask;
    const uint32_t pool_size = this->m_conf_connection_pool_size;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Iterate over the pool, looking for a matching connection with minimum
    // timestamp.
    lock.lock(this->m_pool_mutex);
    this->m_pool.resize(pool_size);
    uniptr<Redis_Connection>* use_slot = nullptr;
    const steady_time now = steady_clock::now();

    for(auto& slot : this->m_pool) {
      if(slot && (now - slot->m_time_pooled > idle_timeout))
        slot.reset();

      if(slot && (!use_slot || (slot->m_time_pooled < (*use_slot)->m_time_pooled))
              && (slot->m_server == server) && (slot->m_port == port)
              && (slot->m_user == user))
        use_slot = &slot;
    }

    if(use_slot)
      return move(*use_slot);

    lock.unlock();

    // Create a new connection.
    cow_string passwd = default_password;
    mask_string(passwd.mut_data(), passwd.size(), nullptr, password_mask);
    return new_uni<Redis_Connection>(server, port, user, passwd);
  }

bool
Redis_Connector::
pool_connection(uniptr<Redis_Connection>&& conn) noexcept
  {
    if(!conn)
      return false;

    if(!conn->m_reset_clear) {
      // If `.reset()` has not been called or has failed, the connection cannot
      // be reused safely, so ignore the request.
      POSEIDON_LOG_ERROR(("Redis connection not reset properly"));
      return false;
    }

    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Put the connection back into an empty slot.
    lock.lock(this->m_pool_mutex);
    uniptr<Redis_Connection>* use_slot = nullptr;
    const steady_time now = steady_clock::now();

    for(auto& slot : this->m_pool) {
      if(slot && (now - slot->m_time_pooled > idle_timeout))
        slot.reset();

      if(!slot || !use_slot || (slot->m_time_pooled < (*use_slot)->m_time_pooled))
        use_slot = &slot;
    }

    if(!use_slot)
      return false;

    conn->m_time_pooled = steady_clock::now();
    *use_slot = move(conn);
    return true;
  }

}  // namespace poseidon
