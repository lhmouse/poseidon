// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_header_parser.hpp"
using namespace ::poseidon;

int
main()
  {
    HTTP_Header_Parser parser;
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == false);
    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    // `Alt-Svc`; attributes
    parser.reload(&"h3=\":443\"; ma=2592000,h3-29=\":443\"; ma=2592001");
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "ma");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == "2592000");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_integer() == 2592000);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_double() == 2592000);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3-29");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "ma");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == "2592001");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_integer() == 2592001);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_double() == 2592001);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == false);
    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    // `Alt-Svc`; multiple sections
    parser.reload(&"h3=\":443\"; ma=2592000,h3-29=\":443\"; ma=2592000");
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3-29");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == false);
    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    // `Alt-Svc`; attributes with bad whitespace
    parser.reload(&"h3 = \":443\" ; ma = 2592000 , h3-29 = \":443\" ; ma = 2592001");
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "ma");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_integer() == 2592000);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_double() == 2592000);

    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3-29");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "ma");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_integer() == 2592001);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_double() == 2592001);

    POSEIDON_TEST_CHECK(parser.next_element() == false);
    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    // `Alt-Svc`; multiple sections with bad whitespace
    parser.reload(&"h3 = \":443\" ; ma = 2592000 , h3-29 = \":443\" ; ma = 2592001");
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "h3-29");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ":443");

    POSEIDON_TEST_CHECK(parser.next_element() == false);
    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    // `Set-Cookie`; unquoted date/time
    parser.reload(&"1P_JAR=2023-06-30-10; expires=Sun, 30-Jul-2023 10:00:45 GMT; path=/; domain=.google.com; Secure");
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "1P_JAR");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == "2023-06-30-10");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "expires");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == true);
    POSEIDON_TEST_CHECK(parser.current_value().as_system_time() == system_clock::from_time_t(1690711245));

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "path");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == "/");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "domain");
    POSEIDON_TEST_CHECK(parser.current_value().as_string() == ".google.com");
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_attribute() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "Secure");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == false);
    POSEIDON_TEST_CHECK(parser.next_attribute() == false);

    // `Cache-Control`
    parser.reload(&"private, no-cache, proxy-revalidate");
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "private");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "no-cache");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "proxy-revalidate");
    POSEIDON_TEST_CHECK(parser.current_value().is_null());
    POSEIDON_TEST_CHECK(parser.current_value().is_integer() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_double() == false);
    POSEIDON_TEST_CHECK(parser.current_value().is_datetime() == false);

    POSEIDON_TEST_CHECK(parser.next_element() == false);
    POSEIDON_TEST_CHECK(parser.next_attribute() == false);
  }
