// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_field_name.hpp"
using namespace ::poseidon;

int
main()
  {
    HTTP_Field_Name n1, n2;
    POSEIDON_TEST_CHECK(n1.empty());
    POSEIDON_TEST_CHECK(n1.size() == 0);
    POSEIDON_TEST_CHECK(n1 == "");
    POSEIDON_TEST_CHECK(n1 == n2);
    POSEIDON_TEST_CHECK(n1.rdhash() == n2.rdhash());

    n1 = &"Hello-world";
    POSEIDON_TEST_CHECK(!n1.empty());
    POSEIDON_TEST_CHECK(n1.size() == 11);
    POSEIDON_TEST_CHECK(n1 == "Hello-world");
    POSEIDON_TEST_CHECK(n1 == "hello-world");
    POSEIDON_TEST_CHECK(n1 == "HELLO-WORLD");
    POSEIDON_TEST_CHECK(n1 != n2);
    POSEIDON_TEST_CHECK(n1.rdhash() != n2.rdhash());

    n2 = &"HELLO-WORLD";
    POSEIDON_TEST_CHECK(n1 == n2);
    POSEIDON_TEST_CHECK(n1.rdhash() == n2.rdhash());

    n1.canonicalize();
    POSEIDON_TEST_CHECK(::memcmp(n1.data(), "hello-world", 11) == 0);
    POSEIDON_TEST_CHECK(n1 == n2);
    POSEIDON_TEST_CHECK(n1.rdhash() == n2.rdhash());

    n1.clear();
    POSEIDON_TEST_CHECK(n1.empty());
    POSEIDON_TEST_CHECK(n1.size() == 0);
    POSEIDON_TEST_CHECK(n1 == "");
    POSEIDON_TEST_CHECK(n1 != n2);
    POSEIDON_TEST_CHECK(n1.rdhash() != n2.rdhash());

    n2.clear();
    POSEIDON_TEST_CHECK(n1 == n2);
    POSEIDON_TEST_CHECK(n1.rdhash() == n2.rdhash());

    n1 = &"abc";
    n2 = &"abD";
    POSEIDON_TEST_CHECK(n1 != n2);
    POSEIDON_TEST_CHECK(n1 < n2);
    POSEIDON_TEST_CHECK(n1 <= n2);
    POSEIDON_TEST_CHECK(n2 > n1);
    POSEIDON_TEST_CHECK(n2 >= n1);

    n1 = &"abc";
    n2 = &"ABCz";
    POSEIDON_TEST_CHECK(n1 != n2);
    POSEIDON_TEST_CHECK(n1 < n2);
    POSEIDON_TEST_CHECK(n1 <= n2);
    POSEIDON_TEST_CHECK(n2 > n1);
    POSEIDON_TEST_CHECK(n2 >= n1);
  }
