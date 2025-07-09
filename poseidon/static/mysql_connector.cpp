// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_connector.hpp"
#include "../mysql/mysql_connection.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
#include <rocket/once_flag.hpp>
namespace poseidon {
namespace {

struct Pooled_Connection
  {
    steady_time time;
    uniptr<MySQL_Connection> conn;
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(MySQL_Connector,
  Pooled_Connection);

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
    // Read the server name and password from configuration. The MySQL client
    // library is able to perform DNS lookup as necessary, so this need not be
    // an IP address.
    cow_string default_service_uri = conf_file.get_string_opt(
                                &"mysql.default_service_uri").value_or(&"root@localhost:3306/admin");
    cow_string default_password = conf_file.get_string_opt(
                                        &"mysql.default_password").value_or(&"");

    // Read the secondary server (replica) from configuration. If not configured
    // it defaults to the primary one.
    cow_string secondary_service_uri = conf_file.get_string_opt(
                                &"mysql.secondary_service_uri").value_or(default_service_uri);
    cow_string secondary_password = conf_file.get_string_opt(
                                        &"mysql.secondary_password").value_or(default_password);

    // Read the tertiary server (replica) from configuration. If not configured
    // it defaults to the secondary one.
    cow_string tertiary_service_uri = conf_file.get_string_opt(
                                &"mysql.tertiary_service_uri").value_or(secondary_service_uri);
    cow_string tertiary_password = conf_file.get_string_opt(
                                        &"mysql.tertiary_password").value_or(secondary_password);

    // Read connection pool settings from configuration.
    uint32_t connection_pool_size = static_cast<uint32_t>(conf_file.get_integer_opt(
                                        &"mysql.connection_pool_size", 0, 100).value_or(0));
    seconds connection_idle_timeout = seconds(static_cast<int>(conf_file.get_integer_opt(
                                        &"mysql.connection_idle_timeout", 0, 86400).value_or(60)));

    // Initialize the MySQL client library in a thread-safe manner.
    static ::rocket::once_flag s_init_once;
    s_init_once.call(::mysql_library_init, 0, nullptr, nullptr);

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    this->m_conf_default_service_uri.swap(default_service_uri);
    this->m_conf_default_password.swap(default_password);
    this->m_conf_secondary_service_uri.swap(secondary_service_uri);
    this->m_conf_secondary_password.swap(secondary_password);
    this->m_conf_tertiary_service_uri.swap(tertiary_service_uri);
    this->m_conf_tertiary_password.swap(tertiary_password);
    this->m_conf_connection_pool_size = static_cast<uint32_t>(connection_pool_size);
    this->m_conf_connection_idle_timeout = static_cast<seconds>(connection_idle_timeout);
  }

POSEIDON_VISIBILITY_HIDDEN
uniptr<MySQL_Connection>
MySQL_Connector::
do_get_pooled_connection_opt(seconds idle_timeout, const cow_string& service_uri)
  {
    plain_mutex::unique_lock lock(this->m_pool_mutex);
    const steady_time now = steady_clock::now();

    // Look for a matching connection.
    uniptr<MySQL_Connection> conn;
    for(auto pos = this->m_pool.mut_begin();  pos != this->m_pool.end();  ++pos)
      if(pos->conn->m_service_uri == service_uri) {
        conn.swap(pos->conn);
        this->m_pool.erase(pos);
        break;
      }

    // Close idle connections.
    while(!this->m_pool.empty() && (now - this->m_pool.back().time > idle_timeout))
      this->m_pool.pop_back();

    return conn;
  }

uniptr<MySQL_Connection>
MySQL_Connector::
allocate_connection(const cow_string& service_uri, const cow_string& password)
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    auto conn = this->do_get_pooled_connection_opt(idle_timeout, service_uri);
    if(!conn)
      conn = new_uni<MySQL_Connection>(service_uri, password);
    return conn;
  }

uniptr<MySQL_Connection>
MySQL_Connector::
allocate_default_connection()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const cow_string service_uri = this->m_conf_default_service_uri;
    const cow_string password = this->m_conf_default_password;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    auto conn = this->do_get_pooled_connection_opt(idle_timeout, service_uri);
    if(!conn)
      conn = new_uni<MySQL_Connection>(service_uri, password);
    return conn;
  }

uniptr<MySQL_Connection>
MySQL_Connector::
allocate_secondary_connection()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const cow_string service_uri = this->m_conf_secondary_service_uri;
    const cow_string password = this->m_conf_secondary_password;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    auto conn = this->do_get_pooled_connection_opt(idle_timeout, service_uri);
    if(!conn)
      conn = new_uni<MySQL_Connection>(service_uri, password);
    return conn;
  }

uniptr<MySQL_Connection>
MySQL_Connector::
allocate_tertiary_connection()
  {
    plain_mutex::unique_lock lock(this->m_conf_mutex);
    const cow_string service_uri = this->m_conf_tertiary_service_uri;
    const cow_string password = this->m_conf_tertiary_password;
    const seconds idle_timeout = this->m_conf_connection_idle_timeout;
    lock.unlock();

    auto conn = this->do_get_pooled_connection_opt(idle_timeout, service_uri);
    if(!conn)
      conn = new_uni<MySQL_Connection>(service_uri, password);
    return conn;
  }

bool
MySQL_Connector::
pool_connection(uniptr<MySQL_Connection>&& conn)
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

    if(pool_size == 0) {
      // The connection pool is disabled.
      POSEIDON_LOG_WARN(("MySQL connection pool disabled"));
      return false;
    }

    lock.lock(this->m_pool_mutex);
    const steady_time now = steady_clock::now();

    // Close idle connections.
    while(!this->m_pool.empty() && (now - this->m_pool.back().time > idle_timeout))
      this->m_pool.pop_back();

    // Trim the pool.
    while(this->m_pool.size() + 1 > pool_size)
      this->m_pool.pop_back();

    // Insert the connection in the beginning.
    X_Pooled_Connection elem;
    elem.time = now;
    elem.conn = move(conn);
    this->m_pool.insert(this->m_pool.begin(), move(elem));
    return true;
  }

}  // namespace poseidon
