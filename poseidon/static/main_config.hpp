// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_MAIN_CONFIG_
#define POSEIDON_STATIC_MAIN_CONFIG_

#include "../fwd.hpp"
#include "../base/config_file.hpp"
namespace poseidon {

class Main_Config
  {
  private:
    mutable plain_mutex m_mutex;
    Config_File m_file;

  public:
    // Constructs an empty configuration file.
    Main_Config() noexcept;

  public:
    Main_Config(const Main_Config&) = delete;
    Main_Config& operator=(const Main_Config&) & = delete;
    ~Main_Config();

    // Reloads 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload();

    // Copies the current file.
    // Configuration files are reference-counted and cheap to copy.
    // This function is thread-safe.
    Config_File
    copy() noexcept;

    // Copies a raw value.
    // This function is thread-safe.
    ::asteria::Value
    copy_value(chars_view vpath);

    // Copies a boolean value. The value must exist and must be a boolean,
    // otherwise an exception is thrown.
    // This function is thread-safe.
    bool
    copy_boolean(chars_view vpath) const;

    // Copies a boolean value. If a non-null value exists, it must be a boolean,
    // otherwise an exception is thrown.
    // This function is thread-safe.
    opt<bool>
    copy_boolean_opt(chars_view vpath) const;

    // Copies an integer. The value must exist and must be an integer within the
    // given range, otherwise an exception is thrown.
    // This function is thread-safe.
    int64_t
    copy_integer(chars_view vpath, int64_t min, int64_t max) const;

    // Copies an integer. If a non-null value exists, it must be an integer
    // within the given range, otherwise an exception is thrown.
    // This function is thread-safe.
    opt<int64_t>
    copy_integer_opt(chars_view vpath, int64_t min, int64_t max) const;

    // Copies a float-point value. The value must exist and must be a number
    // within the given range, otherwise an exception is thrown.
    // This function is thread-safe.
    double
    copy_real(chars_view vpath, double min, double max) const;

    // Copies a float-point value. If a non-null value exists, it must be a
    // number within the given range, otherwise an exception is thrown.
    // This function is thread-safe.
    opt<double>
    copy_real_opt(chars_view vpath, double min, double max) const;

    // Copies a string. The value must exist and must be a string, otherwise an
    // exception is thrown.
    // This function is thread-safe.
    cow_string
    copy_string(chars_view vpath) const;

    // Copies a string. If a non-null value exists, it must be a string,
    // otherwise an exception is thrown.
    // This function is thread-safe.
    opt<cow_string>
    copy_string_opt(chars_view vpath) const;
  };

}  // namespace poseidon
#endif
