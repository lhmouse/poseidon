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
query(const cow_string& value_path) const
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

#define do_apply_subscript_name_  \
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
            value_path, off, this->m_path);  \
      \
      current = parent->ptr(name);  \
      if(!current)  \
        return ::asteria::null;  \
    }

#define do_apply_subscript_index_  \
    {  \
      const ::asteria::V_array* parent;  \
      if(current->is_array())  \
        parent = &(current->as_array());  \
      else \
        POSEIDON_THROW((  \
            "Invalid value path `$1` at offset `$2`: invalid subscript of non-array",  \
            "[in configuration file '$3']"),  \
            value_path, off, this->m_path);  \
      \
      current = parent->ptr(index);  \
      if(!current)  \
        return ::asteria::null;  \
    }

    for(size_t off = 0;  off <= value_path.size();  ++ off)
      switch(ps)
        {
        case parser_name_i:
          switch(value_path[off])
            {
            case ' ':
            case '\t':
            case '\0':
              ps = parser_name_i;
              continue;

            case 'A' ... 'Z':
            case 'a' ... 'z':
            case '_':
            case '-':
            case '$':
            case '@':
            case '*':
              name.assign(1, value_path[off]);
              ps = parser_name;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: name expected",
                  "[in configuration file '$3']"),
                  value_path, off, this->m_path);
            }

        case parser_name:
          switch(value_path[off])
            {
            case 'A' ... 'Z':
            case 'a' ... 'z':
            case '0' ... '9':
            case '_':
            case '-':
            case '$':
            case '@':
            case '*':
              name.push_back(value_path[off]);
              ps = parser_name;
              continue;

            case ' ':
            case '\t':
            case '\0':
              do_apply_subscript_name_
              ps = parser_name_t;
              continue;

            case '.':
              do_apply_subscript_name_
              ps = parser_name_i;
              continue;

            case '[':
              do_apply_subscript_name_
              ps = parser_index_i;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: invalid character",
                  "[in configuration file '$3']"),
                  value_path, off, this->m_path);
            }

        case parser_name_t:
          switch(value_path[off])
            {
            case ' ':
            case '\t':
            case '\0':
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
                  value_path, off, this->m_path);
            }

        case parser_index_i:
          switch(value_path[off])
            {
            case ' ':
            case '\t':
            case '\0':
              ps = parser_index_i;
              continue;

            case '0' ... '9':
              index = static_cast<uint32_t>(value_path[off] - '0');
              ps = parser_index;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: digit expected",
                  "[in configuration file '$3']"),
                  value_path, off, this->m_path);
            }

        case parser_index:
          switch(value_path[off])
            {
            case '0' ... '9':
              if(index >= 999999)
                POSEIDON_THROW((
                    "Invalid value path `$1` at offset `$2`: integer too large",
                    "[in configuration file '$3']"),
                    value_path, off, this->m_path);

              index = index * 10 + static_cast<uint32_t>(value_path[off] - '0');
              ps = parser_index;
              continue;

            case ' ':
            case '\t':
            case '\0':
              do_apply_subscript_index_
              ps = parser_index_t;
              continue;

            case ']':
              do_apply_subscript_index_
              ps = parser_name_t;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: closed bracket expected",
                  "[in configuration file '$3']"),
                  value_path, off, this->m_path);
            }

        case parser_index_t:
          switch(value_path[off])
            {
            case ' ':
            case '\t':
            case '\0':
              ps = parser_index_t;
              continue;

            case ']':
              ps = parser_name_t;
              continue;

            default:
              POSEIDON_THROW((
                  "Invalid value path `$1` at offset `$2`: closed bracket expected",
                  "[in configuration file '$3']"),
                  value_path, off, this->m_path);
            }

        default:
          ROCKET_ASSERT(false);
        }

    if(ps != parser_name_t)
      POSEIDON_THROW((
          "Invalid value path `$1`: unexpected end of input",
          "[in configuration file '$3']"),
          value_path, this->m_path);

    if(!current)
      POSEIDON_THROW((
          "Invalid value path `$1`: empty path not allowed",
          "[in configuration file '$3']"),
          value_path, this->m_path);

    return *current;
  }

}  // namespace poseidon
