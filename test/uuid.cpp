// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/base/uuid.hpp"
using namespace ::poseidon;

int
main()
  {
    UUID tid;
    POSEIDON_TEST_CHECK(tid == UUID());
    POSEIDON_TEST_CHECK(tid == UUID::min());
    POSEIDON_TEST_CHECK(tid <= UUID::min());
    POSEIDON_TEST_CHECK(tid >= UUID::min());
    POSEIDON_TEST_CHECK(tid != UUID::max());
    POSEIDON_TEST_CHECK(tid < UUID::max());
    POSEIDON_TEST_CHECK(tid <= UUID::max());
    POSEIDON_TEST_CHECK(UUID::max() > tid);
    POSEIDON_TEST_CHECK(UUID::max() >= tid);

    POSEIDON_TEST_CHECK(tid.parse("") == 0);
    POSEIDON_TEST_CHECK(tid.parse("ef8a3765-a4f4-4d24-9c72-ee61d7b1253.") == 0);
    POSEIDON_TEST_CHECK_CATCH(UUID("ef8a3765-a4f4-4d24-9c72-ee61d7b1253."));

    char uuid_dstr[64];
    ::memset(uuid_dstr, '*', sizeof(uuid_dstr));
    POSEIDON_TEST_CHECK(tid.parse("EF8A3765-A4F4-4D24-9C72-EE61D7B1253D") == 36);
    tid.print_partial(uuid_dstr);
    POSEIDON_TEST_CHECK(::strcmp(uuid_dstr, "ef8a3765-a4f4-4d24-9c72-ee61d7b1253d") == 0);

    UUID t1 = UUID::random();
    t1.print_partial(uuid_dstr);
    ::fprintf(stderr, "t1 = %s\n", uuid_dstr);
    POSEIDON_TEST_CHECK(UUID::min() < t1);
    POSEIDON_TEST_CHECK(UUID::max() > t1);

    UUID t2 = UUID::random();
    t2.print_partial(uuid_dstr);
    ::fprintf(stderr, "t2 = %s\n", uuid_dstr);
    POSEIDON_TEST_CHECK(t2 > t1);
    POSEIDON_TEST_CHECK(UUID::min() < t2);
    POSEIDON_TEST_CHECK(UUID::max() > t2);

    UUID t3 = UUID::random();
    t3.print_partial(uuid_dstr);
    ::fprintf(stderr, "t3 = %s\n", uuid_dstr);
    POSEIDON_TEST_CHECK(t3 > t2);
    POSEIDON_TEST_CHECK(UUID::min() < t3);
    POSEIDON_TEST_CHECK(UUID::max() > t3);

    POSEIDON_TEST_CHECK(t1.parse("2F6F4C9A-4CFF-4DBE-A483-9E9AE3A1AA63") == 36);
    POSEIDON_TEST_CHECK(t2.parse("2F6F4C9A-4D01-4DBE-82DE-030869CBE90C") == 36);
    POSEIDON_TEST_CHECK(t1 < t2);
  }
