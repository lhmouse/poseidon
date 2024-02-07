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

    hval.set_integer(1234567890123456789);
    POSEIDON_TEST_CHECK(hval.is_integer());
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456789);
    hval.mut_integer() += 1;
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456790);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "1234567890123456790");

    hval.set_double(42.5);
    POSEIDON_TEST_CHECK(hval.is_double());
    POSEIDON_TEST_CHECK(hval.as_double() == 42.5);
    hval.mut_double() += 1;
    POSEIDON_TEST_CHECK(hval.as_double() == 43.5);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "43.5");

    hval.set_string(&"meow");
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.as_string() == "meow");
    hval.mut_string() += "MEOW";
    POSEIDON_TEST_CHECK(hval.as_string() == "meowMEOW");
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'meowMEOW\'");

    hval.set_mysql_datetime(2016,7,21, 16,26,51,678);
    POSEIDON_TEST_CHECK(hval.is_mysql_time());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'2016-07-21 16:26:51.678\'");

    hval.clear();
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "NULL");

    hval.set_string(&"hello\r\n\tworld");
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'hello\\r\\n\tworld\'");

    hval.set_string(&"with\1control");
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\'with\1control\'");
  }
