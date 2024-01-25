// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "config_file.hpp"
#include "../utils.hpp"
#include <stdlib.h>
#include <asteria/library/system.hpp>
namespace poseidon {

Config_File::
Config_File(cow_stringR path)
  {
    // Resolve the path to an absolute one.
    ::rocket::unique_ptr<char, void (void*)> abs_path(::free);
    if(!abs_path.reset(::realpath(path.safe_c_str(), nullptr)))
      POSEIDON_THROW((
          "Could not find configuration file '$1'",
          "[`realpath()` failed: ${errno:full}]"),
          path);

    // Read the file.
    this->m_path.append(abs_path.get());
    this->m_root = ::asteria::std_system_load_conf(this->m_path);
  }

Config_File::
~Config_File()
  {
  }

const ::asteria::Value&
Config_File::
query(initializer_list<cow_string> path) const
  {
    // We would like to return a `Value`, so the path shall not be empty.
    auto cur = path.begin();
    if(cur == path.end())
      POSEIDON_THROW(("Empty value path not valid"));

    // Resolve the first segment.
    auto parent = &(this->m_root);
    auto value = parent->ptr(*cur);

    // Resolve all remaining segments.
    while(value && (++cur != path.end())) {
      if(value->is_null())
        return ::asteria::null_value;

      if(!value->is_object())
        POSEIDON_THROW((
            "Unexpected type of `$1` (expecting an `object`, got `$2`)",
            "[in configuration file '$3']"),
            implode(path.begin(), (size_t) (cur - path.begin), '.'), *value,
            this->m_path);

      // Descend into this child object.
      parent = &(value->as_object());
      value = parent->ptr(*cur);
    }

    if(!value)
      return ::asteria::null_value;

    return *value;
  }

}  // namespace poseidon
