// This file is part of Poseidon.
// Copyright (C) 2022-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/http/http_method.hpp"
using namespace ::poseidon;

int
main()
  {
    HTTP_Method method;
    POSEIDON_TEST_CHECK(method.as_number() == http_method_null);
    POSEIDON_TEST_CHECK(method == http_method_null);

#define do_check_http_method_(X)  \
    POSEIDON_CHECK(method.parse(#X) == strlen(#X));  \
    POSEIDON_TEST_CHECK(method.as_number() == http_method_##X);  \
    POSEIDON_TEST_CHECK(method == http_method_##X);  \
    POSEIDON_TEST_CHECK(method.to_string() == #X);

    do_check_http_method_(DELETE)
    do_check_http_method_(GET)
    do_check_http_method_(HEAD)
    do_check_http_method_(POST)
    do_check_http_method_(PUT)
    do_check_http_method_(CONNECT)
    do_check_http_method_(OPTIONS)
    do_check_http_method_(TRACE)
    do_check_http_method_(COPY)
    do_check_http_method_(LOCK)
    do_check_http_method_(MKCOL)
    do_check_http_method_(MOVE)
    do_check_http_method_(PROPFIND)
    do_check_http_method_(PROPPATCH)
    do_check_http_method_(SEARCH)
    do_check_http_method_(UNLOCK)
    do_check_http_method_(BIND)
    do_check_http_method_(REBIND)
    do_check_http_method_(UNBIND)
    do_check_http_method_(ACL)
    do_check_http_method_(REPORT)
    do_check_http_method_(MKACTIVITY)
    do_check_http_method_(CHECKOUT)
    do_check_http_method_(MERGE)

    POSEIDON_CHECK(method.parse("M-SEARCH") == 8);
    POSEIDON_TEST_CHECK(method.as_number() == http_method_MSEARCH);
    POSEIDON_TEST_CHECK(method == http_method_MSEARCH);
    POSEIDON_TEST_CHECK(method.to_string() == "M-SEARCH");

    do_check_http_method_(NOTIFY)
    do_check_http_method_(SUBSCRIBE)
    do_check_http_method_(UNSUBSCRIBE)
    do_check_http_method_(PATCH)
    do_check_http_method_(PURGE)
    do_check_http_method_(MKCALENDAR)
    do_check_http_method_(LINK)
    do_check_http_method_(UNLINK)
    do_check_http_method_(SOURCE)
    do_check_http_method_(QUERY)
  }
