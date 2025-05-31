// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

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
    Mongo_Connection conn(&"root@localhost:27017/admin", &"123456");
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
    format(fmt, "$1-->\n", doc);

    while(conn.fetch_reply(doc)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", doc);
    }

    // `db.system.version.find()`
    doc.clear();
    doc.emplace_back(&"find", &"system.version");
    conn.execute(doc);

    num = 0;
    format(fmt, "$1-->\n", doc);

    while(conn.fetch_reply(doc)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", doc);
    }

    // `db.system.version.find({_id:{$regex:'\w*Version$'}})`
    doc.clear();
    doc.emplace_back(&"find", &"system.version");
    auto& filter = doc.emplace_back(&"filter", nullptr).second.mut_document();
    auto& _id = filter.emplace_back(&"_id", nullptr).second.mut_document();
    _id.emplace_back(&"$regex", &R"(^\w+Schema$)");
    conn.execute(doc);

    num = 0;
    format(fmt, "$1-->\n", doc);

    while(conn.fetch_reply(doc)) {
      format(fmt, "[$1] --->\n", ++num);
      format(fmt, "  $1\n", doc);
    }

    ::fprintf(stderr, "reset ==> %d\n", conn.reset());
  }
