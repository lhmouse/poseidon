// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_RESPONSE_HEADERS_
#define POSEIDON_HTTP_HTTP_RESPONSE_HEADERS_

#include "../fwd.hpp"
#include "http_value.hpp"
namespace poseidon {

class HTTP_Response_Headers
  {
  private:
    uint32_t m_status;
    cow_string m_reason;
    cow_bivector<cow_string, HTTP_Value> m_headers;

  public:
    // Constructors
    explicit
    HTTP_Response_Headers(uint32_t status) noexcept
      : m_status(status)
      { }

    explicit
    HTTP_Response_Headers(uint32_t status, cow_stringR reason) noexcept
      : m_status(status), m_reason(reason)
      { }

    HTTP_Response_Headers&
    swap(HTTP_Response_Headers& other) noexcept
      {
        ::std::swap(this->m_status, other.m_status);
        this->m_reason.swap(other.m_reason);
        this->m_headers.swap(other.m_headers);
        return *this;
      }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(HTTP_Response_Headers);

    // Accesses raw data.
    uint32_t
    status() const noexcept
      { return this->m_status;  }

    void
    set_status(uint32_t status) noexcept
      { this->m_status = status;  }

    cow_stringR
    reason() const noexcept
      { return this->m_reason;  }

    void
    set_reason(cow_stringR reason) noexcept
      { this->m_reason = reason;  }

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

    template<typename ValueT>
    void
    push_header(cow_stringR name, ValueT&& value)
      { this->m_headers.emplace_back(name, ::std::forward<ValueT>(value));  }

    void
    pop_header() noexcept
      { this->m_headers.pop_back();  }

    cow_stringR
    header_name(size_t index) const
      { return this->m_headers.at(index).first;  }

    const HTTP_Value&
    header_value(size_t index) const
      { return this->m_headers.at(index).second;  }

    bool
    header_name_equals(size_t index, cow_stringR cmp) const
      {
        const auto& my = this->m_headers.at(index).first;
        return ::rocket::ascii_ci_equal(my.data(), my.size(), cmp.data(), cmp.size());
      }

    template<typename ValueT>
    void
    set_header(size_t index, cow_stringR name, ValueT&& value)
      {
        this->m_headers.mut(index).first = name;
        this->m_headers.mut(index).second = ::std::forward<ValueT>(value);
      }

    void
    erase_header(size_t index)
      {
        this->m_headers.erase(index, 1);
      }

    // Writes response headers in raw format, which can be sent through a
    // stream socket. Lines are separated by CR LF pairs.
    tinyfmt&
    print(tinyfmt& fmt) const;

    cow_string
    print_to_string() const;
  };

inline
void
swap(HTTP_Response_Headers& lhs, HTTP_Response_Headers& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline
tinyfmt&
operator<<(tinyfmt& fmt, const HTTP_Response_Headers& resp)
  {
    return resp.print(fmt);
  }

}  // namespace poseidon
#endif
