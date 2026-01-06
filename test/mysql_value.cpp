// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mysql/mysql_value.hpp"
using namespace ::poseidon;

int
main()
  {
    ::setenv("TZ", "TEST+01", true);
    ::tzset();
    POSEIDON_TEST_CHECK(::timezone == +3600);
    POSEIDON_TEST_CHECK(::daylight == 0);

    MySQL_Value hval;
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.to_string() == "NULL");

    hval = 1234567890123456789;
    POSEIDON_TEST_CHECK(hval.is_integer());
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456789);
    hval.open_integer() += 1;
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456790);
    POSEIDON_TEST_CHECK(hval.to_string() == "1234567890123456790");

    hval = 42.5;
    POSEIDON_TEST_CHECK(hval.is_double());
    POSEIDON_TEST_CHECK(hval.as_double() == 42.5);
    hval.open_double() += 1;
    POSEIDON_TEST_CHECK(hval.as_double() == 43.5);
    POSEIDON_TEST_CHECK(hval.to_string() == "43.5");

    hval = &"meow";
    POSEIDON_TEST_CHECK(hval.is_blob());
    POSEIDON_TEST_CHECK(hval.as_blob() == "meow");
    hval.open_blob() += "MEOW";
    POSEIDON_TEST_CHECK(hval.as_blob() == "meowMEOW");
    POSEIDON_TEST_CHECK(hval.to_string() == "\'meowMEOW\'");

    hval = system_clock::from_time_t(1469118411);  // `2016-07-21 15:26:51 -0100`
    POSEIDON_TEST_CHECK(hval.is_datetime());
    POSEIDON_TEST_CHECK(hval.to_string() == "\'2016-07-21 15:26:51\'");

    hval.clear();
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.to_string() == "NULL");

    hval = &"hello\r\n\tworld";
    POSEIDON_TEST_CHECK(hval.is_blob());
    POSEIDON_TEST_CHECK(hval.to_string() == "\'hello\r\n\tworld\'");

    hval = &"with\1control";
    POSEIDON_TEST_CHECK(hval.is_blob());
    POSEIDON_TEST_CHECK(hval.to_string() == "\'with\1control\'");
  }
