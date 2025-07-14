// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_CONFIG_FILE_
#define POSEIDON_BASE_CONFIG_FILE_

#include "../fwd.hpp"
#include <asteria/value.hpp>
namespace poseidon {

class Config_File
  {
  private:
    cow_string m_path;
    ::asteria::V_object m_root;

  public:
    // Constructs an empty file.
    Config_File()
      noexcept;

    // Loads the file denoted by `path`.
    explicit Config_File(const cow_string& conf_path);

    Config_File&
    swap(Config_File& other)
      noexcept
      {
        this->m_path.swap(other.m_path);
        this->m_root.swap(other.m_root);
        return *this;
      }

  public:
    Config_File(const Config_File&) noexcept = default;
    Config_File(Config_File&&) noexcept = default;
    Config_File& operator=(const Config_File&) & noexcept = default;
    Config_File& operator=(Config_File&&) & noexcept = default;
    ~Config_File();

    // Returns the absolute file path.
    // If no file has been loaded, an empty string is returned.
    cow_string
    path()
      const noexcept
      { return this->m_path;  }

    // Gets the root object.
    ::asteria::V_object
    root()
      const noexcept
      { return this->m_root;  }

    // Clears existent data.
    void
    clear()
      noexcept;

    // Loads the file denoted by `path`. In case of an error, an exception is
    // thrown, and there is no effect.
    void
    reload(const cow_string& conf_path);

    // Gets a value denoted by a path, which may look like `foo.bar.value` or
    // `foo.bar[42].value` to access individual fields of objects or arrays.
    // Space characters are ignored. Double quotes are not allowed and are not
    // necessary. The path must not otherwise be empty. If the path does not
    // denote an existent value, a statically allocated null value is returned.
    // If during path resolution, an attempt is made to get a field of a
    // non-object, an exception is thrown.
    const ::asteria::Value&
    query(chars_view vpath)
      const;

    // Gets a boolean value. The value must exist and must be a boolean,
    // otherwise an exception is thrown.
    bool
    get_boolean(chars_view vpath)
      const;

    // Gets a boolean value. If a non-null value exists, it must be a boolean,
    // otherwise an exception is thrown.
    opt<bool>
    get_boolean_opt(chars_view vpath)
      const;

    // Gets an integer. The value must exist and must be an integer within the
    // given range, otherwise an exception is thrown.
    int64_t
    get_integer(chars_view vpath, int64_t min, int64_t max)
      const;

    // Gets an integer. If a non-null value exists, it must be an integer
    // within the given range, otherwise an exception is thrown.
    opt<int64_t>
    get_integer_opt(chars_view vpath, int64_t min, int64_t max)
      const;

    // Gets a float-point value. The value must exist and must be a number
    // within the given range, otherwise an exception is thrown.
    double
    get_real(chars_view vpath, double min, double max)
      const;

    // Gets a float-point value. If a non-null value exists, it must be a
    // number within the given range, otherwise an exception is thrown.
    opt<double>
    get_real_opt(chars_view vpath, double min, double max)
      const;

    // Gets a string. The value must exist and must be a string, otherwise an
    // exception is thrown.
    const cow_string&
    get_string(chars_view vpath)
      const;

    // Gets a string. If a non-null value exists, it must be a string,
    // otherwise an exception is thrown.
    opt<cow_string>
    get_string_opt(chars_view vpath)
      const;

    // Gets the size of an array. The value must exist and must be an array,
    // otherwise an exception is thrown.
    size_t
    get_array_size(chars_view vpath)
      const;

    // Gets the size of an array. If a non-null value exists, it must be an
    // array, otherwise an exception is thrown.
    opt<size_t>
    get_array_size_opt(chars_view vpath)
      const;
  };

inline
void
swap(Config_File& lhs, Config_File& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
