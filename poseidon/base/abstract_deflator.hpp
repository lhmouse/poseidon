// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_ABSTRACT_DEFLATOR_
#define POSEIDON_BASE_ABSTRACT_DEFLATOR_

#include "../fwd.hpp"
#include "../details/zlib_fwd.hpp"
namespace poseidon {

class Abstract_Deflator
  {
  private:
    scoped_deflate_stream m_strm;

  public:
    // Constructs a data compressor.
    explicit
    Abstract_Deflator(zlib_Format format, int level = Z_DEFAULT_COMPRESSION);

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
    do_on_deflate_resize_output_buffer(size_t& size)
      = 0;

    // This callback is invoked to inform derived classes that all input data
    // have been compressed but the output buffer is not full. `backup` is the
    // number of uninitialized bytes in the end of the previous buffer. All the
    // output buffers, other than these uninitialized bytes, have been filled
    // with compressed data.
    // If an exception is thrown, the state of this stream is unspecified.
    virtual
    void
    do_on_deflate_truncate_output_buffer(size_t backup)
      = 0;

  public:
    Abstract_Deflator(const Abstract_Deflator&) = delete;
    Abstract_Deflator& operator=(const Abstract_Deflator&) & = delete;
    virtual ~Abstract_Deflator();

    // Resets internal states. Pending data are discarded.
    void
    reset()
      noexcept;

    // Compresses some data and returns the number of bytes that have been
    // consumed. This function returns zero if `finish()` has been called to
    // complete the current stream.
    size_t
    deflate(chars_view data);

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
