// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "config_file.hpp"
#include "../utils.hpp"
#include <stdlib.h>
#include <asteria/utils.hpp>
#include <asteria/library/system.hpp>
namespace poseidon {

Config_File::
Config_File() noexcept
  {
  }

Config_File::
~Config_File()
  {
  }

void
Config_File::
clear() noexcept
  {
    this->m_path.clear();
    this->m_root.clear();
  }

void
Config_File::
reload(cow_stringR path)
  {
    auto real_path = ::asteria::get_real_path(path);
    auto real_root = ::asteria::std_system_load_conf(real_path);

    // This will not throw exceptions.
    this->m_path.swap(real_path);
    this->m_root.swap(real_root);
  }

const ::asteria::Value&
Config_File::
queryv(const char* first, const char* const* psegs, size_t nsegs) const
  {
    auto pval = this->m_root.ptr(::rocket::sref(first));
    size_t ks = 0;

    while((pval != nullptr) && (ks != nsegs)) {
      if(pval->is_null()) {
        pval = nullptr;
        break;
      }
      else if(!pval->is_object()) {
        // Compose a friendly message.
        cow_string errp;
        errp << first;
        for(size_t k = 0;  k != ks;  ++k)
          errp << '.' << psegs[k];

        POSEIDON_THROW((
            "`$1` is not an object (got `$2`)",
            "[in configuration file '$4']"),
            errp, ::asteria::describe_type(pval->type()), this->m_path);
      }

      pval = pval->as_object().ptr(::rocket::sref(psegs[ks]));
      ks ++;
    }

    if(!pval)
      return ::asteria::null;

    return *pval;
  }

}  // namespace poseidon
