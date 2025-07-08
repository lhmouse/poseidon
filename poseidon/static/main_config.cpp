// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "main_config.hpp"
namespace poseidon {

Main_Config::
Main_Config() noexcept
  {
  }

Main_Config::
~Main_Config()
  {
  }

void
Main_Config::
reload()
  {
    // Read the file.
    Config_File file;
    file.reload(&"main.conf");

    // Set up new data.
    plain_mutex::unique_lock lock(this->m_mutex);
    this->m_file.swap(file);
  }

Config_File
Main_Config::
copy() noexcept
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file;
  }

::asteria::Value
Main_Config::
copy_value(chars_view vpath)
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file.query(vpath);
  }

opt<cow_string>
Main_Config::
copy_string_opt(chars_view vpath)
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file.get_string_opt(vpath);
  }

opt<bool>
Main_Config::
copy_boolean_opt(chars_view vpath)
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file.get_boolean_opt(vpath);
  }

opt<int64_t>
Main_Config::
copy_integer_opt(chars_view vpath, int64_t min, int64_t max)
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file.get_integer_opt(vpath, min, max);
  }

opt<int64_t>
Main_Config::
copy_integer_opt(chars_view vpath)
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file.get_integer_opt(vpath);
  }

opt<double>
Main_Config::
copy_real_opt(chars_view vpath, double min, double max)
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file.get_real_opt(vpath, min, max);
  }

opt<double>
Main_Config::
copy_real_opt(chars_view vpath)
  {
    plain_mutex::unique_lock lock(this->m_mutex);
    return this->m_file.get_real_opt(vpath);
  }

}  // namespace poseidon
