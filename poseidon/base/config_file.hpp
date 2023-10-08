// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

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
    constexpr
    Config_File() noexcept
      :
        m_path(), m_root()
      {
      }

    // Loads the file denoted by `path`.
    explicit
    Config_File(cow_stringR path);

    Config_File&
    swap(Config_File& other) noexcept
      {
        this->m_path.swap(other.m_path);
        this->m_root.swap(other.m_root);
        return *this;
      }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(Config_File);

    // Returns the absolute file path.
    // If no file has been loaded, an empty string is returned.
    cow_string
    path() const noexcept
      { return this->m_path;  }

    // Accesses the contents.
    ::asteria::V_object
    root() const noexcept
      { return this->m_root;  }

    // Gets a value denoted by a path, which shall not be empty.
    // If the path does not denote an existent value, a statically allocated
    // null value is returned. If during path resolution, an attempt is made
    // to get a field of a non-object, an exception is thrown.
    const ::asteria::Value&
    query(initializer_list<phsh_string> path) const;

    template<typename... SegmentT>
    const ::asteria::Value&
    query(const SegmentT&... segs) const
      {
        return this->query({ ::rocket::sref(segs)... });
      }
  };

inline
void
swap(Config_File& lhs, Config_File& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace poseidon
#endif
