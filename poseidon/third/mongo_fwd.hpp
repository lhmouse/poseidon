// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_MONGO_FWD_
#define POSEIDON_THIRD_MONGO_FWD_

#include "../fwd.hpp"
#include <libbson-1.0/bson/bson.h>
#include <libmongoc-1.0/mongoc/mongoc.h>
namespace poseidon {

class scoped_bson
  {
  private:
    ::bson_t m_bson[1];

  public:
    scoped_bson() noexcept
      {
        ::bson_init(this->m_bson);
      }

    ~scoped_bson()
      {
        ::bson_destroy(this->m_bson);
      }

    scoped_bson(const scoped_bson& other) = delete;
    scoped_bson& operator=(const scoped_bson& other) & = delete;

    constexpr operator const ::bson_t*() const noexcept
      { return this->m_bson;  }

    operator ::bson_t*() noexcept
      { return this->m_bson;  }
  };

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

struct mongoc_cursor_deleter
  {
    void
    operator()(::mongoc_cursor_t* p) const noexcept
      { ::mongoc_cursor_destroy(p);  }
  };

using uniptr_mongoc_uri = ::rocket::unique_ptr<::mongoc_uri_t, mongoc_uri_deleter>;
using uniptr_mongoc_client = ::rocket::unique_ptr<::mongoc_client_t, mongoc_client_deleter>;
using uniptr_mongoc_cursor = ::rocket::unique_ptr<::mongoc_cursor_t, mongoc_cursor_deleter>;

}  // namespace poseidon
#endif
