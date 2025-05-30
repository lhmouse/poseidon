// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mysql/mysql_connection.hpp"
#include "../poseidon/mysql/mysql_value.hpp"
#include <rocket/tinyfmt_file.hpp>
using namespace ::poseidon;

int
main()
  {
    ::mysql_library_init(0, nullptr, nullptr);

    // Try connecting to localhost. If the server is offline, skip the test.
    MySQL_Connection conn(&"root@localhost/mysql", &"123456", 0);
    try {
      conn.execute(&"select * from `engine_cost`", {});
    }
    catch(exception& e) {
      ::fprintf(stderr, "could not connect to server: %s\n", e.what());
      return ::strstr(e.what(), "ERROR 2002:")
                  ? 77  // skip
                  :  1; // fail
    }

    cow_vector<cow_string> fields;
    conn.fetch_fields(fields);

    cow_vector<MySQL_Value> values;
    int row = 0;
    ::rocket::tinyfmt_file fmt(stderr, nullptr);

    while(conn.fetch_row(values)) {
      POSEIDON_TEST_CHECK(fields.size() == values.size());
      format(fmt, "[$1] --->\n", ++row);
      for(size_t k = 0;  k != fields.size();  ++k)
        format(fmt, "  $1 = $2\n", fields.at(k), values.at(k));
    }

    ::fprintf(stderr, "reset ==> %d\n", conn.reset());
  }
