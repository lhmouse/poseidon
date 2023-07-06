// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_HEADER_PARSER_
#define POSEIDON_HTTP_HTTP_HEADER_PARSER_

#include "../fwd.hpp"
#include "http_value.hpp"
namespace poseidon {

class HTTP_Header_Parser
  {
  private:
    // source header string
    cow_string m_hstr;
    static constexpr size_t hpos_error = (size_t) -127;
    size_t m_hpos;

    // name and value of current attribute
    cow_string m_name;
    HTTP_Value m_value;

  public:
    // Constructs a parser for a single HTTP header, suitable for parsing
    // various HTTP headers in the semicolon-inside-comma-separated format,
    // such as `Cookie` and `Accept-Encoding`.
    constexpr
    HTTP_Header_Parser() noexcept
      : m_hstr(), m_hpos(), m_name(), m_value()
      { }

    explicit
    HTTP_Header_Parser(cow_stringR hstr) noexcept
      {
        this->reload(hstr);
      }

  private:
    int
    do_next_attribute_from_separator();

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(HTTP_Header_Parser);

    // Has an error occurred?
    bool
    error() const noexcept
      { return this->m_hpos == hpos_error;  }

    // Clears all fields.
    void
    clear() noexcept
      {
        this->m_hstr.clear();
        this->m_hpos = 0;
        this->m_name.clear();
        this->m_value.clear();
      }

    // Reloads a new string. All existent contents are destroyed.
    void
    reload(cow_stringR hstr);

    // Advances to the next attribute of the current element, separated by a
    // semicolon. This function will not advance pass a comma.
    // Returns whether the end of source string has been reached or an error
    // has happened.
    bool
    next_attribute();

    // Advances to the next element, separated by a comma. All in-between
    // attributes are skipped.
    // Returns whether the end of source string has been reached or an error
    // has happened.
    bool
    next_element();

    // Examine the name of the current attribute.
    constexpr
    cow_stringR
    current_name() const noexcept
      { return this->m_name;  }

    cow_string&
    mut_current_name() noexcept
      { return this->m_name;  }

    // Get the value of the current attribute.
    constexpr
    const HTTP_Value&
    current_value() const noexcept
      { return this->m_value;  }

    HTTP_Value&
    mut_current_value() noexcept
      { return this->m_value;  }
  };

}  // namespace poseidon
#endif
