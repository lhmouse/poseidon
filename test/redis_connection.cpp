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
    Redis_Connection conn(&"localhost:6379", &"123456", 0);
    ::std::vector<cow_string> cmds;
    try {
      cmds = { &"echo", &"meow" };
      conn.execute(cmds.data(), cmds.size());
    }
    catch(exception& e) {
      ::fprintf(stderr, "could not connect to server: %s\n", e.what());
      return ::strstr(e.what(), "Connection refused")
                  ? 77  // skip
                  :  1; // fail
    }

    int num = 0;
    ::rocket::tinyfmt_file fmt(stderr, nullptr);
    Redis_Value value;

    // `client list`
    cmds = { &"client", &"list" };
    conn.execute(cmds.data(), cmds.size());

    num = 0;
    format(fmt, "client list -->\n");

    while(conn.fetch_reply(value)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", value);
    }

    // `keys *`
    cmds = { &"keys", &"*" };
    conn.execute(cmds.data(), cmds.size());

    num = 0;
    format(fmt, "keys * -->\n");

    while(conn.fetch_reply(value)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", value);
    }

    ::fprintf(stderr, "reset ==> %d\n", conn.reset());
  }
