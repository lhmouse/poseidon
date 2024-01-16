// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_FRAME_PARSER_
#define POSEIDON_HTTP_WEBSOCKET_FRAME_PARSER_

#include "../fwd.hpp"
namespace poseidon {

class MySQL_Table
  {
  public:
    struct Column
      {
        cow_string name;
        MySQL_Column_Type type = { };
        bool nullable = false;
      };

    struct Index
      {
        cow_string name;
        cow_vector<cow_string> columns;
        bool unique = false;
      };

  private:
    cow_string m_name;
    MySQL_Engine_Type m_engine = { };
    cow_vector<Column> m_columns;
    cow_vector<Index> m_indexes;

  public:
    // Constructs an empty table.
    constexpr
    MySQL_Table() noexcept
      { }

    MySQL_Table&
    swap(MySQL_Table& other) noexcept
      {
        this->m_name.swap(other.m_name);
        this->m_columns.swap(other.m_columns);
        this->m_indexes.swap(other.m_indexes);
        ::std::swap(this->m_engine, other.m_engine);
        return *this;
      }

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(MySQL_Table);

    // Gets and sets the table name.
    // If the table name is not a valid identifier, an exception is thrown, and
    // there is no effect.
    cow_stringR
    name() const noexcept
      { return this->m_name;  }

    void
    set_name(cow_stringR engine);

    // Gets and sets the storage engine.
    // If the storage engine is not valid, an exception is thrown, and there is
    // no effect.
    MySQL_Engine_Type
    engine() const noexcept
      { return this->m_engine;  }

    void
    set_engine(MySQL_Engine_Type xengine);

    // Gets a column.
    size_t
    count_columns() const noexcept
      { return this->m_columns.size();  }

    const Column&
    column(size_t pos) const
      { return this->m_columns.at(pos);  }

    // Adds a column. If a column with the same name already exists, it will be
    // updated in place.
    // Returns the  subscript of the updated column.
    // If `column` references an invalid column, such as an invalid name or type,
    // an exception is thrown, and there is no effect.
    size_t
    add_column(const Column& column);

    // Gets an index.
    size_t
    count_indexes() const noexcept
      { return this->m_indexes.size();  }

    const Index&
    index(size_t pos) const
      { return this->m_indexes.at(pos);  }

    // Adds an index. If an index with the same name already exists, it will be
    // updated in place. If `index.name` equals `"primary"`, it denotes the
    // primary index, and `index.unique` must be `true`.
    // Returns the  subscript of the updated index.
    // If `index` references an invalid index, such as an invalid name or a
    // non-existent column, an exception is thrown, and there is no effect.
    size_t
    add_index(const Index& index);

    // Clears all columns with all indexes.
    void
    clear_columns() noexcept;
  };

inline
void
swap(MySQL_Table& lhs, MySQL_Table& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace poseidon
#endif
