// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/base/uuid.hpp"
using namespace ::poseidon;

int
main()
  {
    UUID tid;
    POSEIDON_TEST_CHECK(tid == UUID());
    POSEIDON_TEST_CHECK(tid == uuid_nil);
    POSEIDON_TEST_CHECK(tid <= uuid_nil);
    POSEIDON_TEST_CHECK(tid >= uuid_nil);
    POSEIDON_TEST_CHECK(tid != uuid_max);
    POSEIDON_TEST_CHECK(tid < uuid_max);
    POSEIDON_TEST_CHECK(tid <= uuid_max);
    POSEIDON_TEST_CHECK(uuid_max > tid);
    POSEIDON_TEST_CHECK(uuid_max >= tid);

    POSEIDON_TEST_CHECK(tid.parse("") == 0);
    POSEIDON_TEST_CHECK(tid.parse("ef8a3765-a4f4-4d24-9c72-ee61d7b1253.") == 0);
    POSEIDON_TEST_CHECK_CATCH(UUID("ef8a3765-a4f4-4d24-9c72-ee61d7b1253."));

    POSEIDON_TEST_CHECK(tid.parse("ef8a3765-a4f4-4d24-9c72-ee61d7b1253d") == 36);
    POSEIDON_TEST_CHECK(tid == POSEIDON_UUID_INIT(ef8a3765,a4f4,4d24,9c72,ee61d7b1253d));

    char uuid_dstr[64];
    ::memset(uuid_dstr, '*', sizeof(uuid_dstr));
    tid.print_partial(uuid_dstr);
    POSEIDON_TEST_CHECK(::strcmp(uuid_dstr, "EF8A3765-A4F4-4D24-9C72-EE61D7B1253D") == 0);

    UUID t1 = UUID::random();
    t1.print_partial(uuid_dstr);
    ::fprintf(stderr, "t1 = %s\n", uuid_dstr);
    POSEIDON_TEST_CHECK(uuid_nil < t1);
    POSEIDON_TEST_CHECK(uuid_max > t1);

    UUID t2 = UUID::random();
    t2.print_partial(uuid_dstr);
    ::fprintf(stderr, "t2 = %s\n", uuid_dstr);
    POSEIDON_TEST_CHECK(t2 > t1);
    POSEIDON_TEST_CHECK(uuid_nil < t2);
    POSEIDON_TEST_CHECK(uuid_max > t2);

    UUID t3 = UUID::random();
    t3.print_partial(uuid_dstr);
    ::fprintf(stderr, "t3 = %s\n", uuid_dstr);
    POSEIDON_TEST_CHECK(t3 > t2);
    POSEIDON_TEST_CHECK(uuid_nil < t3);
    POSEIDON_TEST_CHECK(uuid_max > t3);
  }
