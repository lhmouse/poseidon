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
    inflate_Stream m_strm;

  public:
    // Constructs a data decompressor. `format` shall be `zlib_raw`,
    // `zlib_inflate` or `zlib_gzip`. `wbits` shall be an integer
    // between 9 and 15, inclusively.
    explicit
    Inflator(zlib_Format format, int wbits = 15);

  private:
    void
    do_check_output_buffer();

    void
    do_clear_pointers();

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

    // Gets the inflate stream.
    ::z_stream*
    z_stream() noexcept
      { return this->m_strm;  }

    // Clears internal states. Pending data are discarded.
    void
    clear() noexcept;

    // Decompresses some data and returns the number of bytes that have been
    // consumed. This function may return a value that is less than `size` if
    // an end-of-stream marker has been encountered in the current stream.
    size_t
    inflate(chars_proxy data);

    // Completes the current stream. No data shall be written any further. If
    // no end-of-stream marker has been found in the current stream, `false`
    // is returned.
    bool
    finish();
  };

}  // namespace poseidon
#endif
