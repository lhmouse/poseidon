// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_query_parser.hpp"
using namespace ::poseidon;

int
main()
  {
    HTTP_Query_Parser parser;
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value() == "");

    POSEIDON_TEST_CHECK(parser.next_element() == false);

    parser.reload(&"key1=value1&key2=42&empty=&novalue&%41%42C=a%62c");
    POSEIDON_TEST_CHECK(parser.current_name() == "");
    POSEIDON_TEST_CHECK(parser.current_value() == "");

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "key1");
    POSEIDON_TEST_CHECK(parser.current_value() == "value1");

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "key2");
    POSEIDON_TEST_CHECK(parser.current_value() == "42");

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "empty");
    POSEIDON_TEST_CHECK(parser.current_value() == "");

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "novalue");
    POSEIDON_TEST_CHECK(parser.current_value() == "");

    POSEIDON_TEST_CHECK(parser.next_element() == true);
    POSEIDON_TEST_CHECK(parser.current_name() == "ABC");
    POSEIDON_TEST_CHECK(parser.current_value() == "abc");

    POSEIDON_TEST_CHECK(parser.next_element() == false);
  }
