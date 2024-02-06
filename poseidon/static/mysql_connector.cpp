// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mysql_connector.hpp"
#include "../mysql/enums.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Pooled_Connection
  {
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(MySQL_Connector, Pooled_Connection);

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
    // Parse new configuration. Default ones are defined here.
  }

}  // namespace poseidon
