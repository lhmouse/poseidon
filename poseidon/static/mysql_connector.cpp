// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_connector.hpp"
#include "../mysql/mysql_connection.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <rocket/once_flag.hpp>
namespace poseidon {

MySQL_Connector::
MySQL_Connector() noexcept
  {
  }

MySQL_Connector::
~MySQL_Connector()
  {
  }

void
MySQL_Connector::
reload(const Config_File& conf_file)
  {
    static ::rocket::once_flag s_mysql_init_once;
    s_mysql_init_once.call(::mysql_library_init, 0, nullptr, nullptr);

    // Parse new configuration. Default ones are defined here.
    cow_string default_service_uri = &"root@localhost:3306/admin";
    cow_string default_password;
    uint32_t default_password_mask = 0;
    int64_t connection_pool_size = 0;
    int64_t connection_idle_timeout = 60;

    // Read the server name from configuration. The MySQL client library is able
    // to perform DNS lookup as necessary, so this need not be an IP address.
    auto conf_value = conf_file.query("mysql", "default_service_uri");
    if(conf_value.is_string())
      default_service_uri = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mysql.default_service_uri`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    // Read the password from configuration. The password is not to be stored
    // as plaintext in memory.
    conf_value = conf_file.query("mysql", "default_password");
    if(conf_value.is_string())
      default_password = conf_value.as_string();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mysql.default_password`: expecting a `string`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    default_password_mask = random_uint32() | 0x80;
    mask_string(default_password.mut_data(), default_password.size(), nullptr, default_password_mask);

    // Read the connection pool size from configuration.
    conf_value = conf_file.query("mysql", "connection_pool_size");
    if(conf_value.is_integer())
      connection_pool_size = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mysql.connection_pool_size`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((connection_pool_size < 0) || (connection_pool_size > 100))
      POSEIDON_THROW((
          "`mysql.connection_pool_size` value `$1` out of range",
          "[in configuration file '$2']"),
          connection_pool_size, conf_file.path());

    // Read the idle timeout from configuration, as the number of seconds.
    conf_value = conf_file.query("mysql", "connection_idle_timeout");
    if(conf_value.is_integer())
      connection_idle_timeout = conf_value.as_integer();
    else if(!conf_value.is_null())
      POSEIDON_THROW((
          "Invalid `mysql.connection_idle_timeout`: expecting an `integer`, got `$1`",
          "[in configuration file '$2']"),
          conf_value, conf_file.path());

    if((connection_idle_timeout < 0) || (connection_idle_timeout > 8640000))
      POSEIDON_THROW((
          "`mysql.connection_idle_timeout` value `$1` out of range",
          "[in configuration file '$2']"),
          connection_idle_timeout, conf_file.path());

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_default_service_uri.swap(default_service_uri);
    this->m_conf_default_password.swap(default_password);
    this->m_conf_default_password_mask = default_password_mask;
    this->m_conf_connection_pool_size = static_cast<uint32_t>(connection_pool_size);
    this->m_conf_connection_idle_timeout = static_cast<seconds>(connection_idle_timeout);
  }

uniptr<MySQL_Connection>
MySQL_Connector::
allocate_connection(cow_stringR service_uri, cow_stringR password)
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Look for a matching connection with minimum timestamp.
    lock.lock(this->m_pool_mutex);
    const steady_time now = steady_clock::now();

    auto pos = this->m_pool.mut_begin();
    while((pos != this->m_pool.end()) && (now - (*pos)->m_time_pooled > idle_timeout))
      pos = this->m_pool.erase(pos);

    for(pos = this->m_pool.mut_begin();  pos != this->m_pool.end();  ++pos)
      if((*pos)->m_service_uri == service_uri) {
        auto conn = move(*pos);
        pos = this->m_pool.erase(pos);
        return conn;
      }

    lock.unlock();
    return new_uni<MySQL_Connection>(service_uri, password, 0);
  }

uniptr<MySQL_Connection>
MySQL_Connector::
allocate_default_connection()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const cow_string service_uri = this->m_conf_default_service_uri;
    const cow_string password = this->m_conf_default_password;
    const uint32_t password_mask = this->m_conf_default_password_mask;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Look for a matching connection with minimum timestamp.
    lock.lock(this->m_pool_mutex);
    const steady_time now = steady_clock::now();

    auto pos = this->m_pool.mut_begin();
    while((pos != this->m_pool.end()) && (now - (*pos)->m_time_pooled > idle_timeout))
      pos = this->m_pool.erase(pos);

    for(pos = this->m_pool.mut_begin();  pos != this->m_pool.end();  ++pos)
      if((*pos)->m_service_uri == service_uri) {
        auto conn = move(*pos);
        pos = this->m_pool.erase(pos);
        return conn;
      }

    lock.unlock();
    return new_uni<MySQL_Connection>(service_uri, password, password_mask);
  }

bool
MySQL_Connector::
pool_connection(uniptr<MySQL_Connection>&& conn) noexcept
  {
    if(!conn)
      return false;

    if(!conn->m_reset_clear) {
      // If `.reset()` has not been called or has failed, the connection cannot
      // be reused safely, so ignore the request.
      POSEIDON_LOG_ERROR(("MySQL connection not reset properly"));
      return false;
    }

    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const uint32_t pool_size = this->m_conf_connection_pool_size;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    // Append the connection to the end.
    lock.lock(this->m_pool_mutex);
    const steady_time now = steady_clock::now();

    auto pos = this->m_pool.mut_begin();
    while((pos != this->m_pool.end()) && (now - (*pos)->m_time_pooled > idle_timeout))
      pos = this->m_pool.erase(pos);

    while(this->m_pool.end() - pos >= static_cast<ptrdiff_t>(pool_size))
      pos = this->m_pool.erase(pos);

    conn->m_time_pooled = steady_clock::now();
    this->m_pool.emplace_back(move(conn));
    return true;
  }

}  // namespace poseidon
