// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/redis/redis_connection.hpp"
#include "../poseidon/redis/redis_value.hpp"
#include <rocket/tinyfmt_file.hpp>
using namespace ::poseidon;

int
main()
  {
    // Try connecting to localhost. If the server is offline, skip the test.
    Redis_Connection conn(&"localhost:6379", &"123456");
    cow_vector<cow_string> cmd;
    try {
      cmd = { &"echo", &"meow" };
      conn.execute(cmd);
    }
    catch(exception& e) {
      ::fprintf(stderr, "could not connect to server: %s\n", e.what());
      return ::strstr(e.what(), "Connection refused")
                  ? 77  // skip
                  :  1; // fail
    }

    int num = 0;
    ::rocket::tinyfmt_file fmt(stderr, nullptr);
    cow_string status;
    Redis_Value value;

    // `scan 0 match *`
    cmd = { &"scan", &"0", &"match", &"*" };
    conn.execute(cmd);

    num = 0;
    format(fmt, "scan 0 match * -->\n");

    while(conn.fetch_reply(status, value)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", value);
    }

    ::fprintf(stderr, "reset ==> %d\n", conn.reset());
  }
