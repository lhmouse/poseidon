// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mongo/mongo_connection.hpp"
#include "../poseidon/mongo/mongo_value.hpp"
#include <rocket/tinyfmt_file.hpp>
using namespace ::poseidon;

int
main()
  {
    ::mongoc_init();

    // Try connecting to localhost. If the server is offline, skip the test.
    Mongo_Document doc;
    Mongo_Connection conn(&"localhost", 27017, &"admin", &"root", &"123456");
    try {
      doc.emplace_back(&"ping", 1);
      conn.execute(doc);
    }
    catch(exception& e) {
      ::fprintf(stderr, "could not connect to server: %s\n", e.what());
      return ::strstr(e.what(), "ERROR 15.13053:")
                  ? 77  // skip
                  :  1; // fail
    }

    int num = 0;
    ::rocket::tinyfmt_file fmt(stderr, nullptr);

    // `show dbs`
    doc.clear();
    doc.emplace_back(&"listDatabases", 1);
    doc.emplace_back(&"$db", &"admin");
    conn.execute(doc);

    num = 0;
    format(fmt, "show dbs -->\n");

    while(conn.fetch_reply(doc)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", doc);
    }

    // `db.system.version.find()`
    doc.clear();
    doc.emplace_back(&"find", &"system.version");
    conn.execute(doc);

    num = 0;
    format(fmt, "db.system.version.find() -->\n");

    while(conn.fetch_reply(doc)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", doc);
    }

    ::fprintf(stderr, "reset ==> %d\n", conn.reset());
  }
