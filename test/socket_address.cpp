// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/socket/socket_address.hpp"
using namespace ::poseidon;

int
main()
  {
    Socket_Address addr;
    void* const data = &(addr.mut_addr());
    constexpr size_t size = sizeof(addr.mut_addr());

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("1.2.128.255:12345") == 17);
    POSEIDON_TEST_CHECK(::memcmp(data,
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\x01\x02\x80\xFF", 16) == 0);
    POSEIDON_TEST_CHECK(addr.port() == 12345);

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("3.2.128.255:") == 0);
    POSEIDON_TEST_CHECK(::memcmp(data,
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0);
    POSEIDON_TEST_CHECK(addr.port() == 0);

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("3.2.128.255") == 0);
    POSEIDON_TEST_CHECK(::memcmp(data,
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0);
    POSEIDON_TEST_CHECK(addr.port() == 0);

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("") == 0);
    POSEIDON_TEST_CHECK(::memcmp(data,
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0);
    POSEIDON_TEST_CHECK(addr.port() == 0);

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("[fe80:1234:5678::90ab:cdef]:54321") == 33);
    POSEIDON_TEST_CHECK(::memcmp(data,
        "\xfe\x80\x12\x34\x56\x78\x00\x00\x00\x00\x00\x00\x90\xAB\xCD\xEF", 16) == 0);
    POSEIDON_TEST_CHECK(addr.port() == 54321);

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("[::]:7890") == 9);
    POSEIDON_TEST_CHECK(::memcmp(data,
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0);
    POSEIDON_TEST_CHECK(addr.port() == 7890);

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("[fe80:1254:5678::90ab:cdef]:0") == 29);
    POSEIDON_TEST_CHECK(::memcmp(data,
        "\xfe\x80\x12\x54\x56\x78\x00\x00\x00\x00\x00\x00\x90\xAB\xCD\xEF", 16) == 0);
    POSEIDON_TEST_CHECK(addr.port() == 0);
  }
