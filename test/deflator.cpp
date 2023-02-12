// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/base/deflator.hpp"
using namespace ::poseidon;

struct my_Deflator : Deflator
  {
    cow_string buf;

    explicit
    my_Deflator(int level)
      : Deflator(zlib_format_raw, level)  { }

    virtual
    pair<char*, size_t>
    do_on_deflate_get_output_buffer() override
      {
        size_t old_size = this->buf.size();
        this->buf.append(20, '*');
        char* p = this->buf.mut_data() + old_size;
        size_t n = this->buf.size() - old_size;
        ::fprintf(stderr, "created buffer: %p, %zd\n", p, n);
        return { p, n };
      }

    virtual
    void
    do_on_deflate_truncate_output_buffer(size_t nbackup)
      {
        this->buf.erase(this->buf.size() - nbackup);
        ::fprintf(stderr, "truncated buffer: -%zu\n", nbackup);
      }
  };

int
main()
  {
    // https://www.rfc-editor.org/rfc/rfc7692
    // compression test
    auto defl = ::std::make_unique<my_Deflator>(9);

    POSEIDON_TEST_CHECK(defl->buf.empty());
    defl->deflate("He", 2);
    defl->deflate("llo", 3);
    defl->sync_flush();
    POSEIDON_TEST_CHECK(defl->buf.size() == 11);
    POSEIDON_TEST_CHECK(::memcmp(defl->buf.data(),
        "\xf2\x48\xcd\xc9\xc9\x07\x00\x00\x00\xFF\xFF", 11) == 0);

    // context takeover
    defl->buf.clear();
    defl->deflate("Hello", 5);
    defl->sync_flush();
    POSEIDON_TEST_CHECK(defl->buf.size() == 9);
    POSEIDON_TEST_CHECK(::memcmp(defl->buf.data(),
        "\xf2\x00\x11\x00\x00\x00\x00\xFF\xFF", 9) == 0);

    // end of stream
    defl->buf.clear();
    defl->deflate("Hello", 5);
    defl->finish();
    POSEIDON_TEST_CHECK(defl->buf.size() == 3);
    POSEIDON_TEST_CHECK(::memcmp(defl->buf.data(),
        "\x03\x13\x00", 3) == 0);

    // reset
    defl->clear();
    defl->buf.clear();
    defl->deflate("He", 2);
    defl->deflate("llo", 3);
    defl->sync_flush();
    POSEIDON_TEST_CHECK(defl->buf.size() == 11);
    POSEIDON_TEST_CHECK(::memcmp(defl->buf.data(),
        "\xf2\x48\xcd\xc9\xc9\x07\x00\x00\x00\xFF\xFF", 11) == 0);

    // uncompressed data test
    defl = ::std::make_unique<my_Deflator>(0);
    defl->deflate("He", 2);
    defl->deflate("llo", 3);
    defl->sync_flush();
    POSEIDON_TEST_CHECK(defl->buf.size() == 15);
    POSEIDON_TEST_CHECK(::memcmp(defl->buf.data(),
        "\x00\x05\x00\xfa\xff\x48\x65\x6c\x6c\x6f\x00\x00\x00\xFF\xFF", 15) == 0);
  }
