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
    // Constructs a data decompressor.
    explicit
    Inflator(zlib_Format format);

  protected:
    // This callback is invoked to request an output buffer if none has been
    // requested, or when the previous output buffer is full. Derived classes
    // shall return a buffer of at least `size` bytes where decompressed data
    // will be written, or throw an exception if the request cannot be honored.
    // `size` may be updated to reflect the real size of the output buffer, but
    // its value shall not be decreased.
    // If an exception is thrown, the state of this stream is unspecified.
    virtual
    char*
    do_on_inflate_get_output_buffer(size_t& size) = 0;

    // This callback is invoked to inform derived classes that all input data
    // have been decompressed but the output buffer is not full. `backup` is
    // the number of uninitialized bytes in the end of the previous buffer. All
    // the output buffers, other than these uninitialized bytes, have been
    // filled with decompressed data.
    // If an exception is thrown, the state of this stream is unspecified.
    virtual
    void
    do_on_inflate_truncate_output_buffer(size_t backup) = 0;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Inflator);

    // Resets internal states. Pending data are discarded.
    void
    reset() noexcept;

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
