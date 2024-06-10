// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_HTTP_HEADER_PARSER_
#define POSEIDON_HTTP_HTTP_HEADER_PARSER_

#include "../fwd.hpp"
#include "http_field_name.hpp"
#include "http_value.hpp"
namespace poseidon {

class HTTP_Header_Parser
  {
  private:
    // source header string
    cow_string m_hstr;
    static constexpr size_t error_hpos = (size_t) -127;
    size_t m_hpos = 0;

    // name and value of current attribute
    HTTP_Field_Name m_name;
    HTTP_Value m_value;

  public:
    // Constructs a parser for a single HTTP header, suitable for parsing
    // various HTTP headers in the semicolon-inside-comma-separated format,
    // such as `Cookie` and `Accept-Encoding`.
    HTTP_Header_Parser() noexcept;

  private:
    int
    do_next_attribute_from_separator();

  public:
    HTTP_Header_Parser(const HTTP_Header_Parser&) = delete;
    HTTP_Header_Parser& operator=(const HTTP_Header_Parser&) & = delete;
    ~HTTP_Header_Parser();

    // Has an error occurred?
    bool
    error() const noexcept
      { return this->m_hpos == error_hpos;  }

    // Clears all fields.
    void
    clear() noexcept;

    // Reloads a new string. All existent contents are destroyed.
    void
    reload(cow_stringR hstr);

    // Advances to the next attribute of the current element, separated by a
    // semicolon. This function will not advance pass a comma.
    // Returns whether an attribute has been fetched and no error has occurred.
    bool
    next_attribute();

    // Advances to the next element, separated by a comma. All in-between
    // attributes are skipped.
    // Returns whether an element has been fetched and no error has occurred.
    bool
    next_element();

    // Get the name of the current attribute.
    const HTTP_Field_Name&
    current_name() const noexcept
      { return this->m_name;  }

    HTTP_Field_Name&
    mut_current_name() noexcept
      { return this->m_name;  }

    // Get the value of the current attribute.
    const HTTP_Value&
    current_value() const noexcept
      { return this->m_value;  }

    HTTP_Value&
    mut_current_value() noexcept
      { return this->m_value;  }
  };

}  // namespace poseidon
#endif
