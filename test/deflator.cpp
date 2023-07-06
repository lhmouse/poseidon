// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/easy/easy_deflator.hpp"
using namespace ::poseidon;

int
main()
  {
    // https://www.rfc-editor.org/rfc/rfc7692
    // compression test
    Easy_Deflator defl;
    POSEIDON_TEST_CHECK(defl.output_size() == 0);
    defl.open(zlib_format_raw);
    POSEIDON_TEST_CHECK(defl.deflate("He", 2) == 2);
    POSEIDON_TEST_CHECK(defl.deflate("llo", 3) == 3);
    POSEIDON_TEST_CHECK(defl.sync_flush() == true);
    POSEIDON_TEST_CHECK(defl.output_size() == 11);
    POSEIDON_TEST_CHECK(::memcmp(defl.output_data(),
        "\xf2\x48\xcd\xc9\xc9\x07\x00\x00\x00\xFF\xFF", 11) == 0);

    // context takeover
    defl.output_clear();
    POSEIDON_TEST_CHECK(defl.deflate("Hello", 5) == 5);
    POSEIDON_TEST_CHECK(defl.sync_flush() == true);
    POSEIDON_TEST_CHECK(defl.output_size() == 9);
    POSEIDON_TEST_CHECK(::memcmp(defl.output_data(),
        "\xf2\x00\x11\x00\x00\x00\x00\xFF\xFF", 9) == 0);

    // end of stream
    defl.output_clear();
    POSEIDON_TEST_CHECK(defl.deflate("Hello", 5) == 5);
    POSEIDON_TEST_CHECK(defl.finish() == true);
    POSEIDON_TEST_CHECK(defl.output_size() == 3);
    POSEIDON_TEST_CHECK(::memcmp(defl.output_data(),
        "\x03\x13\x00", 3) == 0);

    defl.output_clear();
    POSEIDON_TEST_CHECK(defl.deflate("Hello", 5) == 0);
    POSEIDON_TEST_CHECK(defl.finish() == true);
    POSEIDON_TEST_CHECK(defl.output_size() == 0);
    POSEIDON_TEST_CHECK(defl.sync_flush() == true);
    POSEIDON_TEST_CHECK(defl.output_size() == 0);

    // reset
    defl.clear();
    POSEIDON_TEST_CHECK(defl.output_size() == 0);
    defl.open(zlib_format_raw);
    POSEIDON_TEST_CHECK(defl.deflate("He", 2) == 2);
    POSEIDON_TEST_CHECK(defl.deflate("llo", 3) == 3);
    POSEIDON_TEST_CHECK(defl.sync_flush() == true);
    POSEIDON_TEST_CHECK(defl.output_size() == 11);
    POSEIDON_TEST_CHECK(::memcmp(defl.output_data(),
        "\xf2\x48\xcd\xc9\xc9\x07\x00\x00\x00\xFF\xFF", 11) == 0);

    // uncompressed data test
    defl.clear();
    POSEIDON_TEST_CHECK(defl.output_size() == 0);
    defl.open(zlib_format_raw, 0);
    POSEIDON_TEST_CHECK(defl.deflate("He", 2) == 2);
    POSEIDON_TEST_CHECK(defl.deflate("llo", 3) == 3);
    POSEIDON_TEST_CHECK(defl.sync_flush() == true);
    POSEIDON_TEST_CHECK(defl.output_size() == 15);
    POSEIDON_TEST_CHECK(::memcmp(defl.output_data(),
        "\x00\x05\x00\xfa\xff\x48\x65\x6c\x6c\x6f\x00\x00\x00\xFF\xFF", 15) == 0);
  }
