// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MONGODB_ENUMS_
#define POSEIDON_MONGODB_ENUMS_

#include "../fwd.hpp"
#include "enums.hpp"
namespace poseidon {

enum Mongo_Value_Type : uint8_t
  {
    mongo_value_null      = 0,  // BSON_TYPE_NULL
    mongo_value_boolean   = 1,  // BSON_TYPE_BOOL
    mongo_value_integer   = 2,  // BSON_TYPE_INT32, BSON_TYPE_INT64
    mongo_value_double    = 3,  // BSON_TYPE_DOUBLE
    mongo_value_utf8      = 4,  // BSON_TYPE_UTF8
    mongo_value_binary    = 5,  // BSON_TYPE_BINARY
    mongo_value_array     = 6,  // BSON_TYPE_ARRAY
    mongo_value_document  = 7,  // BSON_TYPE_DOCUMENT
    mongo_value_oid       = 8,  // BSON_TYPE_OID
    mongo_value_datetime  = 9,  // BSON_TYPE_DATE_TIME
  };

}  // namespace poseidon
#endif
