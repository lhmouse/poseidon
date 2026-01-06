// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_DETAILS_MONGO_FWD_
#define POSEIDON_DETAILS_MONGO_FWD_

#include "../fwd.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <bson/bson.h>
#include <mongoc/mongoc.h>
#pragma GCC diagnostic pop
namespace poseidon {

class scoped_bson
  {
  private:
    mutable ::bson_t m_bson[1];

  public:
    scoped_bson()
      noexcept
      {
        ::bson_init(this->m_bson);
      }

    ~scoped_bson()
      {
        ::bson_destroy(this->m_bson);
      }

    scoped_bson(const scoped_bson& other) = delete;
    scoped_bson& operator=(const scoped_bson& other) & = delete;

    operator ::bson_t*()
      const noexcept
      { return this->m_bson;  }

    const uint8_t*
    data()
      const noexcept
      { return ::bson_get_data(this->m_bson);  }

    uint32_t
    size()
      const noexcept
      { return this->m_bson->len;  }
  };

struct mongoc_uri_deleter
  {
    void
    operator()(::mongoc_uri_t* p)
      const noexcept
      { ::mongoc_uri_destroy(p);  }
  };

struct mongoc_client_deleter
  {
    void
    operator()(::mongoc_client_t* p)
      const noexcept
      { ::mongoc_client_destroy(p);  }
  };

struct mongoc_cursor_deleter
  {
    void
    operator()(::mongoc_cursor_t* p)
      const noexcept
      { ::mongoc_cursor_destroy(p);  }
  };

using uniptr_mongoc_uri = ::rocket::unique_ptr<::mongoc_uri_t, mongoc_uri_deleter>;
using uniptr_mongoc_client = ::rocket::unique_ptr<::mongoc_client_t, mongoc_client_deleter>;
using uniptr_mongoc_cursor = ::rocket::unique_ptr<::mongoc_cursor_t, mongoc_cursor_deleter>;

}  // namespace poseidon
#endif
