// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "mongodb_connector.hpp"
#include "../base/config_file.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Pooled_Connection
  {
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(MongoDB_Connector, Pooled_Connection);

MongoDB_Connector::
MongoDB_Connector() noexcept
  {
  }

MongoDB_Connector::
~MongoDB_Connector()
  {
  }

void
MongoDB_Connector::
reload(const Config_File& conf_file)
  {
    // Parse new configuration. Default ones are defined here.
  }

}  // namespace poseidon
