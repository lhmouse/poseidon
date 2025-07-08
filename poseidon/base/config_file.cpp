// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

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
Config_File(const cow_string& conf_path)
  {
    this->reload(conf_path);
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
reload(const cow_string& conf_path)
  {
    auto real_path = ::asteria::get_real_path(conf_path);
    auto real_root = ::asteria::std_system_load_conf(real_path);

    // This will not throw exceptions.
    this->m_path.swap(real_path);
    this->m_root.swap(real_root);
  }

const ::asteria::Value&
Config_File::
query(chars_view vpath) const
  {
    enum Parser_State
      {
                         // "  main . array [ 123 ]  . value  "
        parser_name_i,   //  ^^     ^^               ^^
        parser_name,     //    ^^^^   ^^^^^            ^^^^^
        parser_name_t,   //        ^       ^      ^^^       ^^
        parser_index_i,  //                 ^^
        parser_index,    //                   ^^^
        parser_index_t,  //                      ^
      };

    const ::asteria::Value* current = nullptr;
    Parser_State ps = parser_name_i;
    cow_string name;
    uint32_t index = 0;
    size_t offset = SIZE_MAX;

#define do_apply_subscript_name_no_semicolon_  \
    {  \
      const ::asteria::V_object* parent;  \
      if(!current)  \
        parent = &(this->m_root);  \
      else if(current->is_object())  \
        parent = &(current->as_object());  \
      else \
        POSEIDON_THROW((  \
            "Invalid value path `$1` at offset `$2`: invalid subscript of non-object",  \
            "[in configuration file '$3']"),  \
            vpath, offset, this->m_path);  \
      \
      current = parent->ptr(name);  \
      if(!current)  \
        return ::asteria::null;  \
    }

#define do_apply_subscript_index_no_semicolon_  \
    {  \
      const ::asteria::V_array* parent;  \
      if(current->is_array())  \
        parent = &(current->as_array());  \
      else \
        POSEIDON_THROW((  \
            "Invalid value path `$1` at offset `$2`: invalid subscript of non-array",  \
            "[in configuration file '$3']"),  \
            vpath, offset, this->m_path);  \
      \
      current = parent->ptr(index);  \
      if(!current)  \
        return ::asteria::null;  \
    }

    while(++ offset != vpath.n)
      switch(ps)
        {
        case parser_name_i:
          switch(vpath.p[offset])
            {
            case ' ':
            case '\t':
              ps = parser_name_i;
              continue;

            case 'A' ... 'Z':
            case 'a' ... 'z':
            case '_':
            case '-':
            case '$':
            case '@':
            case '*':
              name.assign(1, vpath.p[offset]);
              ps = parser_name;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: name expected",
                  "[in configuration file '$3']"),
                  vpath, offset, this->m_path);
            }

        case parser_name:
          switch(vpath.p[offset])
            {
            case 'A' ... 'Z':
            case 'a' ... 'z':
            case '0' ... '9':
            case '_':
            case '-':
            case '$':
            case '@':
            case '*':
              name.push_back(vpath.p[offset]);
              ps = parser_name;
              continue;

            case ' ':
            case '\t':
              do_apply_subscript_name_no_semicolon_
              ps = parser_name_t;
              continue;

            case '.':
              do_apply_subscript_name_no_semicolon_
              ps = parser_name_i;
              continue;

            case '[':
              do_apply_subscript_name_no_semicolon_
              ps = parser_index_i;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: invalid character",
                  "[in configuration file '$3']"),
                  vpath, offset, this->m_path);
            }

        case parser_name_t:
          switch(vpath.p[offset])
            {
            case ' ':
            case '\t':
              ps = parser_name_t;
              continue;

            case '.':
              ps = parser_name_i;
              continue;

            case '[':
              ps = parser_index_i;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: invalid character",
                  "[in configuration file '$3']"),
                  vpath, offset, this->m_path);
            }

        case parser_index_i:
          switch(vpath.p[offset])
            {
            case ' ':
            case '\t':
              ps = parser_index_i;
              continue;

            case '0' ... '9':
              index = static_cast<uint32_t>(vpath.p[offset] - '0');
              ps = parser_index;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: digit expected",
                  "[in configuration file '$3']"),
                  vpath, offset, this->m_path);
            }

        case parser_index:
          switch(vpath.p[offset])
            {
            case '0' ... '9':
              if(index >= 999999)
                POSEIDON_THROW((
                    "Invalid value path `$1` at offset `$2`: integer too large",
                    "[in configuration file '$3']"),
                    vpath, offset, this->m_path);

              index = index * 10 + static_cast<uint32_t>(vpath.p[offset] - '0');
              ps = parser_index;
              continue;

            case ' ':
            case '\t':
              do_apply_subscript_index_no_semicolon_
              ps = parser_index_t;
              continue;

            case ']':
              do_apply_subscript_index_no_semicolon_
              ps = parser_name_t;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: closed bracket expected",
                  "[in configuration file '$3']"),
                  vpath, offset, this->m_path);
            }

        case parser_index_t:
          switch(vpath.p[offset])
            {
            case ' ':
            case '\t':
              ps = parser_index_t;
              continue;

            case ']':
              ps = parser_name_t;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: closed bracket expected",
                  "[in configuration file '$3']"),
                  vpath, offset, this->m_path);
            }

        default:
          ROCKET_ASSERT(false);
        }

    if(ps == parser_name)
      do_apply_subscript_name_no_semicolon_
    else if(ps != parser_name_t)
      POSEIDON_THROW((
          "Invalid value path `$1`: unexpected end of input",
          "[in configuration file '$3']"),
          vpath, this->m_path);

    if(!current)
      POSEIDON_THROW((
          "Invalid value path `$1`: empty path not allowed",
          "[in configuration file '$3']"),
          vpath, this->m_path);

    return *current;
  }

opt<cow_string>
Config_File::
get_string_opt(chars_view vpath) const
  {
    auto value = this->query(vpath);
    if(value.is_null())
      return nullopt;

    if(!value.is_string())
      POSEIDON_THROW((
          "Invalid `$1`: expecting a `string`, got `$2`",
          "[in configuration file '$3']"),
          vpath, value, this->m_path);

    return value.as_string();
  }

opt<bool>
Config_File::
get_boolean_opt(chars_view vpath) const
  {
    auto value = this->query(vpath);
    if(value.is_null())
      return nullopt;

    if(!value.is_boolean())
      POSEIDON_THROW((
          "Invalid `$1`: expecting a `boolean`, got `$2`",
          "[in configuration file '$3']"),
          vpath, value, this->m_path);

    return value.as_boolean();
  }

opt<int64_t>
Config_File::
get_integer_opt(chars_view vpath, int64_t min, int64_t max) const
  {
    auto value = this->query(vpath);
    if(value.is_null())
      return nullopt;

    if(!value.is_integer())
      POSEIDON_THROW((
          "Invalid `$1`: expecting an `integer`, got `$2`",
          "[in configuration file '$3']"),
          vpath, value, this->m_path);

    if(!((value.as_integer() >= min) && (value.as_integer() <= max)))
      POSEIDON_THROW((
          "Invalid `$1`: value `$2` out of range [$4,$5]",
          "[in configuration file '$3']"),
          vpath, value, this->m_path, min, max);

    return value.as_integer();
  }

opt<int64_t>
Config_File::
get_integer_opt(chars_view vpath) const
  {
    auto value = this->query(vpath);
    if(value.is_null())
      return nullopt;

    if(!value.is_integer())
      POSEIDON_THROW((
          "Invalid `$1`: expecting an `integer`, got `$2`",
          "[in configuration file '$3']"),
          vpath, value, this->m_path);

    return value.as_integer();
  }

opt<double>
Config_File::
get_real_opt(chars_view vpath, double min, double max) const
  {
    auto value = this->query(vpath);
    if(value.is_null())
      return nullopt;

    if(!value.is_real())
      POSEIDON_THROW((
          "Invalid `$1`: expecting an `integer` or `real`, got `$2`",
          "[in configuration file '$3']"),
          vpath, value, this->m_path);

    if(!((value.as_real() >= min) && (value.as_real() <= max)))
      POSEIDON_THROW((
          "Invalid `$1`: value `$2` out of range [$4,$5]",
          "[in configuration file '$3']"),
          vpath, value, this->m_path, min, max);

    return value.as_real();
  }

opt<double>
Config_File::
get_real_opt(chars_view vpath) const
  {
    auto value = this->query(vpath);
    if(value.is_null())
      return nullopt;

    if(!value.is_real())
      POSEIDON_THROW((
          "Invalid `$1`: expecting an `integer` or `real`, got `$2`",
          "[in configuration file '$3']"),
          vpath, value, this->m_path);

    return value.as_real();
  }

}  // namespace poseidon
