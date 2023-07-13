// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_DEFLATOR_
#define POSEIDON_BASE_DEFLATOR_

#include "../fwd.hpp"
#include "../third/zlib_fwd.hpp"
namespace poseidon {

class Deflator
  {
  private:
    deflate_Stream m_strm;

  public:
    // Constructs a data compressor. `format` shall be `zlib_raw`, `zlib_deflate`
    // or `zlib_gzip`. `level` shall be an integer between `0` (no compression)
    // and `9` (best compression), or `-1` to select the default value. `wbits`
    // shall be an integer between `9` and `15`, inclusively.
    explicit
    Deflator(zlib_Format format, int level = -1, int wbits = 15);

  private:
    void
    do_check_output_buffer();

    void
    do_clear_pointers();

  protected:
    // This callback is invoked to request an output buffer if none has been
    // requested, or when the previous output buffer is full. Derived classes
    // shall return a temporary memory region where compressed data will be
    // written, or throw an exception if the request cannot be honored. This
    // function shall ensure that there are at least 12 bytes available in the
    // output buffer, otherwise the behavior may be unpredictable.
    // If an exception is thrown, the state of this stream is unspecified.
    virtual
    pair<char*, size_t>
    do_on_deflate_get_output_buffer() = 0;

    // This callback is invoked to inform derived classes that all input data
    // have been compressed but the output buffer is not full. `nbackup` is the
    // number of uninitialized bytes in the end of the previous buffer. All the
    // output buffers, other than these uninitialized bytes, have been filled
    // with compressed data.
    // If an exception is thrown, the state of this stream is unspecified.
    virtual
    void
    do_on_deflate_truncate_output_buffer(size_t nbackup) = 0;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Deflator);

    // Gets the deflate stream.
    ::z_stream*
    z_stream() noexcept
      { return this->m_strm;  }

    // Clears internal states. Pending data are discarded.
    void
    clear() noexcept;

    // Compresses some data and returns the number of bytes that have been
    // consumed. This function returns zero if `finish()` has been called to
    // complete the current stream.
    size_t
    deflate(chars_proxy data);

    // Completes the current deflate block. The effect of this function is
    // described by zlib about its `Z_SYNC_FLUSH` argument.
    bool
    sync_flush();

    // Completes the current deflate block. The effect of this function is
    // described by zlib about its `Z_FULL_FLUSH` argument.
    bool
    full_flush();

    // Completes the current stream. No data shall be written any further. The
    // effect of this function is described by zlib about its `Z_FINISH`
    // argument.
    bool
    finish();
  };

}  // namespace poseidon
#endif
