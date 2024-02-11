// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mysql/mysql_value.hpp"
using namespace ::poseidon;

int
main()
  {
    MySQL_Value hval;
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "NULL");

    hval = 1234567890123456789;
    POSEIDON_TEST_CHECK(hval.is_integer());
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456789);
    hval.mut_integer() += 1;
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456790);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "1234567890123456790");

    hval = 42.5;
    POSEIDON_TEST_CHECK(hval.is_double());
    POSEIDON_TEST_CHECK(hval.as_double() == 42.5);
    hval.mut_double() += 1;
    POSEIDON_TEST_CHECK(hval.as_double() == 43.5);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "43.5");

    hval = &"meow";
    POSEIDON_TEST_CHECK(hval.is_blob());
    POSEIDON_TEST_CHECK(hval.as_blob() == "meow");
    hval.mut_blob() += "MEOW";
    POSEIDON_TEST_CHECK(hval.as_blob() == "meowMEOW");
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'meowMEOW\'");

    ::MYSQL_TIME myt = { };
    myt.year = 2016;
    myt.month = 7;
    myt.day = 21;
    myt.hour = 16;
    myt.minute = 26;
    myt.second = 51;
    myt.second_part = 678;
    hval = myt;
    POSEIDON_TEST_CHECK(hval.is_mysql_time());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'2016-07-21 16:26:51.678\'");

    hval.clear();
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "NULL");

    hval = &"hello\r\n\tworld";
    POSEIDON_TEST_CHECK(hval.is_blob());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'hello\\r\\n\tworld\'");

    hval = &"with\1control";
    POSEIDON_TEST_CHECK(hval.is_blob());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'with\1control\'");
  }
