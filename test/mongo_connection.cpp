// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mongo/mongo_connection.hpp"
//#include "../poseidon/mongo/mongo_value.hpp"
#include <rocket/tinyfmt_file.hpp>
using namespace ::poseidon;

int
main()
  {
    ::mongoc_init();

    // Try connecting to localhost. If the server is offline, skip the test.
    BSON obj;
    Mongo_Connection conn(&"localhost", 27017, &"admin", &"root", &"123456");
    try {
      ::bson_append_int32(obj, "ping", -1, 1);
      conn.execute(obj);
    }
    catch(exception& e) {
      ::fprintf(stderr, "could not connect to server: %s\n", e.what());
      if(::strstr(e.what(), "ERROR 15.13053:") != nullptr)
        return 77;
    }

    int num = 0;
    ::rocket::tinyfmt_file fmt(stderr, nullptr);

    // `show dbs`
    ::bson_reinit(obj);
    ::bson_append_int32(obj, "listDatabases", -1, 1);
    ::bson_append_utf8(obj, "$db", -1, "admin", -1);
    conn.execute(obj);

    num = 0;
    format(fmt, "show dbs -->\n");

    while(conn.fetch_reply(obj)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", obj);
    }

    // `db.system.version.find()`
    ::bson_reinit(obj);
    ::bson_append_utf8(obj, "find", -1, "system.version", -1);
    conn.execute(obj);

    num = 0;
    format(fmt, "db.system.version.find() -->\n");

    while(conn.fetch_reply(obj)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", obj);
    }

    ::fprintf(stderr, "reset ==> %d\n", conn.reset());
  }
