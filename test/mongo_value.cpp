// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/mongo/mongo_value.hpp"
using namespace ::poseidon;

int
main()
  {
    Mongo_Value hval;
    POSEIDON_TEST_CHECK(hval.is_null());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "null");

    hval = 1234567890123456789;
    POSEIDON_TEST_CHECK(hval.is_integer());
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456789);
    hval.open_integer() += 1;
    POSEIDON_TEST_CHECK(hval.as_integer() == 1234567890123456790);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "1234567890123456790");

    hval = 42.5;
    POSEIDON_TEST_CHECK(hval.is_double());
    POSEIDON_TEST_CHECK(hval.as_double() == 42.5);
    hval.open_double() += 1;
    POSEIDON_TEST_CHECK(hval.as_double() == 43.5);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "43.5");

    hval = &"meow";
    POSEIDON_TEST_CHECK(hval.is_utf8());
    POSEIDON_TEST_CHECK(hval.as_utf8() == "meow");
    hval.open_utf8() += "ME\tOW";
    POSEIDON_TEST_CHECK(hval.as_utf8() == "meowME\tOW");
    POSEIDON_TEST_CHECK(hval.print_to_string() == R"("meowME\tOW")");

    constexpr uint8_t bin_data[] = "\x02\x7F\x00\xF2\xFC\xFF\xFE";
    hval = cow_bstring(&bin_data);
    POSEIDON_TEST_CHECK(hval.is_binary());
    POSEIDON_TEST_CHECK(::memcmp(hval.as_binary_data(), bin_data, sizeof(bin_data)) == 0);
    POSEIDON_TEST_CHECK(hval.print_to_string() == R"({$base64:"An8A8vz//g=="})");

    Mongo_Array arr_data = { };
    hval = arr_data;
    POSEIDON_TEST_CHECK(hval.is_array());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "[]");

    arr_data = { 32 };
    hval = arr_data;
    POSEIDON_TEST_CHECK(hval.is_array());
    POSEIDON_TEST_CHECK(hval.as_array().at(0).as_integer() == 32);
    POSEIDON_TEST_CHECK(hval.print_to_string() == "[32]");

    arr_data = { 1, false, &"meow" };
    hval = arr_data;
    POSEIDON_TEST_CHECK(hval.is_array());
    POSEIDON_TEST_CHECK(hval.as_array().at(0).as_integer() == 1);
    POSEIDON_TEST_CHECK(hval.as_array().at(1).as_boolean() == false);
    POSEIDON_TEST_CHECK(hval.as_array().at(2).as_utf8() == "meow");
    POSEIDON_TEST_CHECK(hval.print_to_string() == R"([1,false,"meow"])");

    Mongo_Document doc_data;
    hval = doc_data;
    POSEIDON_TEST_CHECK(hval.is_document());
    POSEIDON_TEST_CHECK(hval.print_to_string() == "{}");

    doc_data = { { &"first", false } };
    hval = doc_data;
    POSEIDON_TEST_CHECK(hval.is_document());
    POSEIDON_TEST_CHECK(hval.as_document().at(0).first == "first");
    POSEIDON_TEST_CHECK(hval.as_document().at(0).second.as_boolean() == false);
    POSEIDON_TEST_CHECK(hval.print_to_string() == R"({"first":false})");

    doc_data = { { &"first", &"meow" }, { &"second", 12.5 } };
    hval = doc_data;
    POSEIDON_TEST_CHECK(hval.is_document());
    POSEIDON_TEST_CHECK(hval.as_document().at(0).first == "first");
    POSEIDON_TEST_CHECK(hval.as_document().at(0).second.as_utf8() == "meow");
    POSEIDON_TEST_CHECK(hval.as_document().at(1).first == "second");
    POSEIDON_TEST_CHECK(hval.as_document().at(1).second.as_double() == 12.5);
    POSEIDON_TEST_CHECK(hval.print_to_string() == R"({"first":"meow","second":12.5})");

    ::bson_oid_t oid_data = { 0x65,0xC9,0x8F,0x34,0x2D,0x18,0x60,0xB7,0xBA,0xEF,0x94,0xFB };
    hval = oid_data;
    POSEIDON_TEST_CHECK(hval.is_oid());
    POSEIDON_TEST_CHECK(::bson_oid_equal(&(hval.as_oid()), &oid_data));
    POSEIDON_TEST_CHECK(hval.print_to_string() == R"({$oid:"65c98f342d1860b7baef94fb"})");

    DateTime datetime_data = system_clock::from_time_t(1633635291);
    hval = datetime_data;
    POSEIDON_TEST_CHECK(hval.is_datetime());
    POSEIDON_TEST_CHECK(hval.as_datetime() == datetime_data);
    POSEIDON_TEST_CHECK(hval.print_to_string() == R"({$date:"2021-10-07T19:34:51.000Z"})");
  }
