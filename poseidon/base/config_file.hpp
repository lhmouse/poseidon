// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

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
    Config_File() noexcept;

    Config_File&
    swap(Config_File& other) noexcept
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
    path() const noexcept
      { return this->m_path;  }

    // Gets the root object.
    ::asteria::V_object
    root() const noexcept
      { return this->m_root;  }

    // Clears existent data.
    void
    clear() noexcept;

    // Loads the file denoted by `path`. In case of an error, an exception is
    // thrown, and there is no effect.
    void
    reload(cow_stringR path);

    // Gets a value denoted by a path, which shall not be empty.
    // If the path does not denote an existent value, a statically allocated
    // null value is returned. If during path resolution, an attempt is made
    // to get a field of a non-object, an exception is thrown.
    const ::asteria::Value&
    queryv(const char* first, const char* const* psegs, size_t nsegs) const;

    template<typename... xNext>
    const ::asteria::Value&
    query(const char* first, const xNext&... xnext) const
      {
        const char* segs[] = { xnext... };
        return this->queryv(first, segs, sizeof...(xNext));
      }
  };

inline
void
swap(Config_File& lhs, Config_File& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
