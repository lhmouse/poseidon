// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/redis/redis_value.hpp"
using namespace ::poseidon;

int
main()
  {
    Redis_Value hval;
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.to_string() == "null");

    hval = 1234567890123456789;
    POSEIDON_TEST_CHECK(hval.is_integer());
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456789);
    hval.open_integer() += 1;
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456790);
    POSEIDON_TEST_CHECK(hval.to_string() == "1234567890123456790");

    hval = &"meow";
    POSEIDON_TEST_CHECK(hval.is_string());
    POSEIDON_TEST_CHECK(hval.as_string() == "meow");
    hval.open_string() += "ME\tOW";
    POSEIDON_TEST_CHECK(hval.as_string() == "meowME\tOW");
    POSEIDON_TEST_CHECK(hval.to_string() == R"("meowME\tOW")");

    Redis_Array arr_data = { };
    hval = arr_data;
    POSEIDON_TEST_CHECK(hval.is_array());
    POSEIDON_TEST_CHECK(hval.to_string() == "[]");

    arr_data = { 32 };
    hval = arr_data;
    POSEIDON_TEST_CHECK(hval.is_array());
    POSEIDON_TEST_CHECK(hval.as_array().at(0).as_integer() == 32);
    POSEIDON_TEST_CHECK(hval.to_string() == "[32]");

    arr_data = { 1, &"meow" };
    hval = arr_data;
    POSEIDON_TEST_CHECK(hval.is_array());
    POSEIDON_TEST_CHECK(hval.as_array().at(0).as_integer() == 1);
    POSEIDON_TEST_CHECK(hval.as_array().at(1).as_string() == "meow");
    POSEIDON_TEST_CHECK(hval.to_string() == R"([1,"meow"])");
  }
