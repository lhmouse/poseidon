// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/utils.hpp"
using namespace ::poseidon;

int
main()
  {
    try {
      POSEIDON_THROW((
          "test $1 $2 $$/end"),
          "exception:", 42);
    }
    catch(exception& e) {
      POSEIDON_TEST_CHECK(::std::strstr(e.what(),
          "test exception: 42 $/end") != nullptr);
    }

    POSEIDON_CHECK(1 + 1);
    POSEIDON_CHECK(true);

    try {
      POSEIDON_CHECK(0+0);
    }
    catch(exception& e) {
      POSEIDON_TEST_CHECK(::std::strstr(e.what(),
          "POSEIDON_CHECK: 0+0") != nullptr);
    }

    POSEIDON_CHECK(decode_and_canonicalize_uri_path("") == "/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/") == "/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/bb") == "/aa/bb");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa///bb") == "/aa/bb");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa//bb/") == "/aa/bb/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/./bb") == "/aa/bb");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/../bb") == "/bb");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/bb/.") == "/aa/bb/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/bb/..") == "/aa/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/bb/../..") == "/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/bb/../../") == "/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/bb/../../..") == "/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/bb/../../../") == "/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/%41") == "/aa/A");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/%41%2F") == "/aa/A/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/%41/%2F") == "/aa/A/");
    POSEIDON_CHECK(decode_and_canonicalize_uri_path("/aa/%41/%2Fb") == "/aa/A/b");
  }
