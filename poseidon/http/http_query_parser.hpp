// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_QUERY_PARSER_
#define POSEIDON_HTTP_HTTP_QUERY_PARSER_

#include "../fwd.hpp"
#include "http_field_name.hpp"
#include "http_value.hpp"
namespace poseidon {

class HTTP_Query_Parser
  {
  private:
    // source query string
    cow_string m_hstr;
    static constexpr size_t error_hpos = static_cast<size_t>(-127);
    size_t m_hpos = 0;

    // name and value of current element
    HTTP_Field_Name m_name;
    HTTP_Value m_value;

  public:
    // Constructs a parser for a single HTTP query string, suitable for parsing
    // the query component of a URI or URL, or the body of an HTTP request whose
    // `Content-Type:` equals `application/x-www-form-urlencoded`.
    HTTP_Query_Parser()
      noexcept;

  public:
    HTTP_Query_Parser(const HTTP_Query_Parser&) = delete;
    HTTP_Query_Parser& operator=(const HTTP_Query_Parser&) & = delete;
    ~HTTP_Query_Parser();

    // Has an error occurred?
    bool
    error()
      const noexcept
      { return this->m_hpos == error_hpos;  }

    // Clears all fields.
    void
    clear()
      noexcept;

    // Reloads a new string. All existent contents are destroyed.
    void
    reload(const cow_string& raw_query);

    // Advances to the next element, separated by an ampersand.
    // Returns whether an element has been fetched and no error has occurred.
    bool
    next_element();

    // Get the name of the current element.
    const HTTP_Field_Name&
    current_name()
      const noexcept
      { return this->m_name;  }

    HTTP_Field_Name&
    mut_current_name()
      noexcept
      { return this->m_name;  }

    // Get the value of the current element.
    const HTTP_Value&
    current_value()
      const noexcept
      { return this->m_value;  }

    HTTP_Value&
    mut_current_value()
      noexcept
      { return this->m_value;  }
  };

}  // namespace poseidon
#endif
