// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../poseidon/base/inflator.hpp"
using namespace ::poseidon;

struct my_Inflator : Inflator
  {
    cow_string buf;

    explicit
    my_Inflator()
      : Inflator(zlib_format_raw)  { }

    virtual
    pair<char*, size_t>
    do_on_inflate_get_output_buffer() override
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
    do_on_inflate_truncate_output_buffer(size_t nbackup)
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
    auto infl = ::std::make_unique<my_Inflator>();

    POSEIDON_TEST_CHECK(infl->buf.empty());
    infl->inflate("\xf2\x48\xcd\xc9\xc9", 5);
    infl->inflate("\x07\x00\x00\x00\xFF\xFF", 6);
    POSEIDON_TEST_CHECK(infl->buf == "Hello");

    // context takeover
    infl->buf.clear();
    infl->inflate("\xf2\x00\x11\x00\x00\x00\x00\xFF\xFF", 9);
    POSEIDON_TEST_CHECK(infl->buf == "Hello");

    // end of stream
    infl->buf.clear();
    infl->inflate("\x03\x13\x00", 3);
    infl->finish();
    POSEIDON_TEST_CHECK(infl->buf == "Hello");

    // reset
    infl->clear();
    infl->buf.clear();
    infl->inflate("\xf2\x48\xcd\xc9\xc9\x07\x00\x00\x00\xFF\xFF", 11);
    POSEIDON_TEST_CHECK(infl->buf == "Hello");

    // uncompressed data test
    infl->clear();
    infl->buf.clear();
    infl->inflate("\x00\x05\x00\xfa\xff\x48\x65\x6c\x6c", 9);
    infl->inflate("\x6f\x00\x00\x00\xFF\xFF", 6);
    POSEIDON_TEST_CHECK(infl->buf == "Hello");
  }
