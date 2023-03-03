// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_value.hpp"
using namespace ::poseidon;

int
main()
  {
    HTTP_Value hval;
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "");

    hval.set_number(42.5);
    POSEIDON_TEST_CHECK(hval.is_number());
    POSEIDON_TEST_CHECK(hval.as_number() == 42.5);
    hval.mut_number() += 1;
    POSEIDON_TEST_CHECK(hval.as_number() == 43.5);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "43.5");

    hval.set_string(sref("meow"));
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.as_string() == "meow");
    hval.mut_string() += "MEOW";
    POSEIDON_TEST_CHECK(hval.as_string() == "meowMEOW");
    POSEIDON_TEST_CHECK(hval.print_to_string() == "meowMEOW");

    hval.set_datetime(HTTP_DateTime("Thu, 21 Jul 2016 16:26:51 GMT"));
    POSEIDON_TEST_CHECK(hval.is_datetime());
    POSEIDON_TEST_CHECK(hval.as_datetime().as_seconds() == (seconds) 1469118411);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "Thu, 21 Jul 2016 16:26:51 GMT");

    hval.clear();
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "");

    hval.set_string(sref("hello\r\n\tworld"));
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\"hello world\"");

    hval.set_string(sref("with,comma"));
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "\"with,comma\"");

    POSEIDON_TEST_CHECK(hval.parse("http-token mumble") == 10);
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.as_string() == "http-token");

    POSEIDON_TEST_CHECK(hval.parse("\"escaped string\" mumble") == 16);
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.as_string() == "escaped string");

    POSEIDON_TEST_CHECK(hval.parse("1.5 mumble") == 3);
    POSEIDON_TEST_CHECK(hval.is_number());
    POSEIDON_TEST_CHECK(hval.as_number() == 1.5);

    POSEIDON_TEST_CHECK(hval.parse("1.5z mumble") == 4);
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.as_string() == "1.5z");

    POSEIDON_TEST_CHECK(hval.parse("Thu, 21 Jul 2016 16:26:51 GMT mumble") == 29);
    POSEIDON_TEST_CHECK(hval.is_datetime());
    POSEIDON_TEST_CHECK(hval.as_datetime().as_seconds() == (seconds) 1469118411);

    POSEIDON_TEST_CHECK(hval.parse("Thu, 21 Jul 2016 16:26:51| GMT mumble") == 3);
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.as_string() == "Thu");

    POSEIDON_TEST_CHECK(hval.parse(" mumble") == 0);
    POSEIDON_TEST_CHECK(hval.is_null());
  }
