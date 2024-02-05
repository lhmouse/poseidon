// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/socket/socket_address.hpp"
#include "../poseidon/socket/enums.hpp"
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

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("3.2.128.255") == 0);

    ::memset(data, 0x66, size);
    POSEIDON_TEST_CHECK(addr.parse("") == 0);

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

    Socket_Address cmp;
    cmp.parse("[fe80:1254:5678::90ab:cdef]:0");
    POSEIDON_TEST_CHECK(addr.compare(cmp) == 0);
    POSEIDON_TEST_CHECK(cmp.compare(addr) == 0);
    POSEIDON_TEST_CHECK(addr == cmp);

    addr.parse("[fe80:1254:5678::90ab:cdef]:1");
    POSEIDON_TEST_CHECK(addr.compare(cmp) > 0);
    POSEIDON_TEST_CHECK(cmp.compare(addr) < 0);
    POSEIDON_TEST_CHECK(addr > cmp);

    addr.parse("[fe80:1254:5678::90ab:cdee]:1");
    POSEIDON_TEST_CHECK(addr.compare(cmp) < 0);
    POSEIDON_TEST_CHECK(cmp.compare(addr) > 0);
    POSEIDON_TEST_CHECK(addr < cmp);

    addr.parse("[fe80:1254:5678::90ab:ceee]:1");
    POSEIDON_TEST_CHECK(addr.compare(cmp) > 0);
    POSEIDON_TEST_CHECK(cmp.compare(addr) < 0);
    POSEIDON_TEST_CHECK(addr > cmp);

    addr.parse("[fe70:1254:5678::90ab:cdee]:1");
    POSEIDON_TEST_CHECK(addr.compare(cmp) < 0);
    POSEIDON_TEST_CHECK(cmp.compare(addr) > 0);
    POSEIDON_TEST_CHECK(addr < cmp);

    // classification
    POSEIDON_TEST_CHECK(addr.parse("0.0.0.0:42") == 10);
    POSEIDON_TEST_CHECK(addr.classify() == ip_address_unspecified);

    POSEIDON_TEST_CHECK(addr.parse("10.6.123.84:80") == 14);
    POSEIDON_TEST_CHECK(addr.classify() == ip_address_private);

    POSEIDON_TEST_CHECK(addr.parse("127.0.0.1:443") == 13);
    POSEIDON_TEST_CHECK(addr.classify() == ip_address_loopback);

    POSEIDON_TEST_CHECK(addr.parse("127.0.0.5:445") == 13);
    POSEIDON_TEST_CHECK(addr.classify() == ip_address_loopback);

    POSEIDON_TEST_CHECK(addr.parse("192.168.60.1:1023") == 17);
    POSEIDON_TEST_CHECK(addr.classify() == ip_address_private);

    POSEIDON_TEST_CHECK(addr.parse("225.13.0.9:335") == 14);
    POSEIDON_TEST_CHECK(addr.classify() == ip_address_multicast);
  }
