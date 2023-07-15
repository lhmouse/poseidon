// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "deflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Deflator::
Deflator(zlib_Format format, int level)
  : m_strm(format, 15, level)
  {
  }

Deflator::
~Deflator()
  {
  }

void
Deflator::
reset() noexcept
  {
    this->m_strm.reset();
  }

size_t
Deflator::
deflate(chars_proxy data)
  {
    if(data.n == 0)
      return 0;

    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      constexpr size_t out_request = 128;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_deflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_deflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      err = this->m_strm.deflate(out_ptr, out_end, in_ptr, in_end, Z_NO_FLUSH);

      if(out_ptr != out_end)
        this->do_on_deflate_truncate_output_buffer((size_t) (out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_ERROR }))
        this->m_strm.throw_exception(err, "deflate");
    }
    while((in_ptr != in_end) && (err == Z_OK));

    // Return the number of characters that have been consumed.
    return (size_t) (in_ptr - data.p);
  }

bool
Deflator::
sync_flush()
  {
    const char* in_ptr = "";
    const char* in_end = in_ptr;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      constexpr size_t out_request = 16;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_deflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_deflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      err = this->m_strm.deflate(out_ptr, out_end, in_ptr, in_end, Z_SYNC_FLUSH);

      if(out_ptr != out_end)
        this->do_on_deflate_truncate_output_buffer((size_t) (out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_ERROR }))
        this->m_strm.throw_exception(err, "deflate");
    }
    while(err == Z_OK);

    // Return whether the operation has succeeded.
    return err == Z_BUF_ERROR;
  }

bool
Deflator::
full_flush()
  {
    const char* in_ptr = "";
    const char* in_end = in_ptr;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      constexpr size_t out_request = 16;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_deflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_deflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      err = this->m_strm.deflate(out_ptr, out_end, in_ptr, in_end, Z_FULL_FLUSH);

      if(out_ptr != out_end)
        this->do_on_deflate_truncate_output_buffer((size_t) (out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_ERROR }))
        this->m_strm.throw_exception(err, "deflate");
    }
    while(err == Z_OK);

    // Return whether the operation has succeeded.
    return err == Z_BUF_ERROR;
  }

bool
Deflator::
finish()
  {
    const char* in_ptr = "";
    const char* in_end = in_ptr;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      constexpr size_t out_request = 16;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_deflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_deflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      err = this->m_strm.deflate(out_ptr, out_end, in_ptr, in_end, Z_FINISH);

      if(out_ptr != out_end)
        this->do_on_deflate_truncate_output_buffer((size_t) (out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        this->m_strm.throw_exception(err, "deflate");
    }
    while(err == Z_OK);

    // Return whether the operation has succeeded.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
