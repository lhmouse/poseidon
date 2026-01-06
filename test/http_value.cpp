// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_value.hpp"
using namespace ::poseidon;

int
main()
  {
    HTTP_Value hval;
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.to_string() == "");

    hval = 42.5;
    POSEIDON_TEST_CHECK(hval.is_double());
    POSEIDON_TEST_CHECK(hval.as_double() == 42.5);
    POSEIDON_TEST_CHECK(hval.to_string() == "42.5");

    hval = &"meow";
    POSEIDON_TEST_CHECK(hval.as_string() == "meow");
    POSEIDON_TEST_CHECK(hval.to_string() == "meow");

    hval = system_clock::from_time_t(1469118411);
    POSEIDON_TEST_CHECK(hval.is_datetime());
    POSEIDON_TEST_CHECK(hval.as_system_time() == system_clock::from_time_t(1469118411));
    POSEIDON_TEST_CHECK(hval.to_string() == "Thu, 21 Jul 2016 16:26:51 GMT");

    hval.clear();
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.to_string() == "");

    hval = &"hello\r\n\tworld";
    POSEIDON_TEST_CHECK(hval.to_string() == "\"hello world\"");

    hval = &"with,comma";
    POSEIDON_TEST_CHECK(hval.to_string() == "\"with,comma\"");

    POSEIDON_TEST_CHECK(hval.parse("http-token mumble") == 10);
    POSEIDON_TEST_CHECK(hval.as_string() == "http-token");

    POSEIDON_TEST_CHECK(hval.parse("\"escaped string\" mumble") == 16);
    POSEIDON_TEST_CHECK(hval.as_string() == "escaped string");

    POSEIDON_TEST_CHECK(hval.parse("1.5 mumble") == 3);
    POSEIDON_TEST_CHECK(hval.as_string() == "1.5");
    POSEIDON_TEST_CHECK(hval.is_double());
    POSEIDON_TEST_CHECK(hval.as_double() == 1.5);

    POSEIDON_TEST_CHECK(hval.parse("1.5z mumble") == 4);
    POSEIDON_TEST_CHECK(hval.as_string() == "1.5z");

    POSEIDON_TEST_CHECK(hval.parse("Thu, 21 Jul 2016 16:26:51 GMT mumble") == 29);
    POSEIDON_TEST_CHECK(hval.is_datetime());
    POSEIDON_TEST_CHECK(hval.as_system_time() == system_clock::from_time_t(1469118411));

    POSEIDON_TEST_CHECK(hval.parse("Thu, 21 Jul 2016 16:26:51| GMT mumble") == 3);
    POSEIDON_TEST_CHECK(hval.as_string() == "Thu");

    POSEIDON_TEST_CHECK(hval.parse(" mumble") == 0);
  }
