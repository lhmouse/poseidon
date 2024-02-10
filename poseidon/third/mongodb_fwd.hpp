// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MONGODB_FWD_
#define POSEIDON_THIRD_MONGODB_FWD_

#include "../fwd.hpp"
#include <libbson-1.0/bson/bson.h>
#include <libmongoc-1.0/mongoc/mongoc.h>
namespace poseidon {

class BSON
  {
  protected:
    ::bson_t m_bson[1];

  public:
    BSON() noexcept
      {
        // Initialize `this->m_bson` using inline storage. No dynamic memory
        // is allocated.
        ::bson_init(this->m_bson);
      }

    BSON(const BSON& other) noexcept
      {
        // Initialize `this->m_bson` with copied contents from `other.m_bson`.
        ::bson_copy_to(other.m_bson, this->m_bson);
      }

    BSON(BSON&& other) noexcept
      {
        // Initialize `this->m_bson` with stolen contents from `other.m_bson`.
        // `other.m_bson` is to be destroyed by `bson_steal()` and must be
        // reinitialized.
        ::bson_steal(this->m_bson, other.m_bson);
        ::bson_init(other.m_bson);
      }

    BSON&
    operator=(const BSON& other) & noexcept
      {
        if(this == &other)
          return *this;

        // Perform a destroy-and-reconstruct operation in place.
        ::bson_destroy(this->m_bson);
        ::bson_copy_to(other.m_bson, this->m_bson);
        return *this;
      }

    BSON&
    operator=(BSON&& other) & noexcept
      {
        if(this == &other)
          return *this;

        // Perform a destroy-and-reconstruct operation in place. `other.m_bson`
        // is to be destroyed by `bson_steal()` and must be reinitialized.
        ::bson_destroy(this->m_bson);
        ::bson_steal(this->m_bson, other.m_bson);
        ::bson_init(other.m_bson);
        return *this;
      }

    ~BSON()
      {
        ::bson_destroy(this->m_bson);
      }

    constexpr operator const ::bson_t*() const noexcept
      { return this->m_bson;  }

    operator ::bson_t*() noexcept
      { return this->m_bson;  }
  };


inline
tinyfmt&
operator<<(tinyfmt& fmt, const BSON& bson)
  {
    char* str = ::bson_as_relaxed_extended_json(bson, nullptr);
    const ::rocket::unique_ptr<char, void (void*)> str_guard(str, ::bson_free);
    return fmt << (str ? str : "(invalid BSON)");
  }

struct mongoc_uri_deleter
  {
    void
    operator()(::mongoc_uri_t* p) const noexcept
      { ::mongoc_uri_destroy(p);  }
  };

struct mongoc_client_deleter
  {
    void
    operator()(::mongoc_client_t* p) const noexcept
      { ::mongoc_client_destroy(p);  }
  };

struct mongoc_collection_deleter
  {
    void
    operator()(::mongoc_collection_t* p) const noexcept
      { ::mongoc_collection_destroy(p);  }
  };

struct mongoc_cursor_deleter
  {
    void
    operator()(::mongoc_cursor_t* p) const noexcept
      { ::mongoc_cursor_destroy(p);  }
  };

using uniptr_mongoc_uri = ::rocket::unique_ptr<::mongoc_uri_t, mongoc_uri_deleter>;
using uniptr_mongoc_client = ::rocket::unique_ptr<::mongoc_client_t, mongoc_client_deleter>;
using uniptr_mongoc_collection = ::rocket::unique_ptr<::mongoc_collection_t, mongoc_collection_deleter>;
using uniptr_mongoc_cursor = ::rocket::unique_ptr<::mongoc_cursor_t, mongoc_cursor_deleter>;

}  // namespace poseidon
#endif
