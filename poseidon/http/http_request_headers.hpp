// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_REQUEST_HEADERS_
#define POSEIDON_HTTP_HTTP_REQUEST_HEADERS_

#include "../fwd.hpp"
#include "http_value.hpp"
namespace poseidon {

class HTTP_Request_Headers
  {
  private:
    cow_string m_verb;
    cow_string m_uri;
    cow_bivector<cow_string, HTTP_Value> m_headers;

  public:
    // Constructors
    explicit
    HTTP_Request_Headers(cow_stringR verb, cow_stringR uri) noexcept
      : m_verb(verb), m_uri(uri)
      { }

    HTTP_Request_Headers&
    swap(HTTP_Request_Headers& other) noexcept
      {
        this->m_verb.swap(other.m_verb);
        this->m_uri.swap(other.m_uri);
        this->m_headers.swap(other.m_headers);
        return *this;
      }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(HTTP_Request_Headers);

    // Accesses raw data.
    cow_stringR
    verb() const noexcept
      { return this->m_verb;  }

    void
    set_verb(cow_stringR verb) noexcept
      { this->m_verb = verb;  }

    cow_stringR
    uri() const noexcept
      { return this->m_uri;  }

    void
    set_uri(cow_stringR uri) noexcept
      { this->m_uri = uri;  }

    const cow_bivector<cow_string, HTTP_Value>&
    headers() const noexcept
      { return this->m_headers;  }

    cow_bivector<cow_string, HTTP_Value>&
    mut_headers() noexcept
      { return this->m_headers;  }

    size_t
    count_headers() const noexcept
      { return this->m_headers.size();  }

    void
    clear_headers() noexcept
      { this->m_headers.clear();  }

    void
    resize_headers(size_t count)
      { this->m_headers.resize(count);  }

    cow_stringR
    header_name(size_t index) const
      { return this->m_headers.at(index).first;  }

    bool
    header_name_equals(size_t index, cow_stringR cmp) const
      {
        const auto& my = this->m_headers.at(index).first;
        bool eq = ::rocket::ascii_ci_equal(my.data(), my.size(), cmp.data(), cmp.size());
        return eq;
      }

    const HTTP_Value&
    header_value(size_t index) const
      { return this->m_headers.at(index).second;  }

    template<typename ValueT>
    void
    set_header(size_t index, cow_stringR name, ValueT&& value)
      {
        this->m_headers.mut(index).first = name;
        this->m_headers.mut(index).second = ::std::forward<ValueT>(value);
      }

    template<typename ValueT>
    void
    add_header(cow_stringR name, ValueT&& value)
      {
        this->m_headers.emplace_back(name, ::std::forward<ValueT>(value));
      }

    // Writes request headers in raw format, which can be sent through a
    // stream socket. Lines are separated by CR LF pairs.
    tinyfmt&
    print(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

inline
void
swap(HTTP_Request_Headers& lhs, HTTP_Request_Headers& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const HTTP_Request_Headers& req)
  {
    return req.print(fmt);
  }

}  // namespace poseidon
#endif
