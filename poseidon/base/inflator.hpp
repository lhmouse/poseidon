// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_INFLATOR_
#define POSEIDON_BASE_INFLATOR_

#include "../fwd.hpp"
#include "../third/zlib_fwd.hpp"
namespace poseidon {

class Inflator
  {
  private:
    zlib_Inflate_Stream m_strm;

  public:
    // Constructs a data decompressor. `format` shall be `zlib_format_raw`,
    // `zlib_format_inflate` or `zlib_format_gzip`. `wbits` shall be an integer
    // between 9 and 15, inclusively.
    explicit
    Inflator(zlib_Format format, int wbits = 15);

  private:
    inline
    void
    do_inflate_prepare(const char* data);

    inline
    int
    do_inflate(uint8_t*& end_out, int flush);

    inline
    void
    do_inflate_cleanup(uint8_t* end_out);

  protected:
    // This callback is invoked to request an output buffer if none has been
    // requested, or when the previous output buffer is full. Derived classes
    // shall return a temporary memory region where decompressed data will be
    // written, or throw an exception if the request cannot be honored. Note
    // that if this function returns a buffer that is too small (for example
    // 3 or 4 bytes) then `finish()` will fail.
    // If an exception is thrown, the state of this stream is unspecified.
    virtual
    pair<char*, size_t>
    do_on_inflate_get_output_buffer() = 0;

    // This callback is invoked to inform derived classes that all input data
    // have been decompressed but the output buffer is not full. `nbackup` is
    // the number of uninitialized bytes in the end of the previous buffer. All
    // the output buffers, other than these uninitialized bytes, have been
    // filled with decompressed data.
    // If an exception is thrown, the state of this stream is unspecified.
    virtual
    void
    do_on_inflate_truncate_output_buffer(size_t nbackup) = 0;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Inflator);

    // Clears internal states. Pending data are discarded.
    Inflator&
    clear() noexcept;

    // Decompresses a chunk of data and flushes all output.
    Inflator&
    inflate(const char* data, size_t size);

    // Completes the current stream. No data shall be written any further. This
    // function expects an end-of-stream marker, and throws an exception if no
    // one is encountered.
    Inflator&
    finish();
  };

}  // namespace poseidon
#endif
