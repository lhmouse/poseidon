// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "deflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Deflator::
Deflator(zlib_Format format, int level)
  :
    m_strm(format, 15, level)
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
    ::deflateReset(this->m_strm);
  }

size_t
Deflator::
deflate(chars_view data)
  {
    if(data.n == 0)
      return 0;

    int err;
    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;

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
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::deflate(this->m_strm, Z_NO_FLUSH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      this->do_on_deflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

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
    int err;
    const char* in_ptr = "";
    const char* in_end = in_ptr;

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
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::deflate(this->m_strm, Z_SYNC_FLUSH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      this->do_on_deflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

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
    int err;
    const char* in_ptr = "";
    const char* in_end = in_ptr;

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
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::deflate(this->m_strm, Z_FULL_FLUSH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      this->do_on_deflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

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
    int err;
    const char* in_ptr = "";
    const char* in_end = in_ptr;

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
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::deflate(this->m_strm, Z_FINISH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      this->do_on_deflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        this->m_strm.throw_exception(err, "deflate");
    }
    while(err == Z_OK);

    // Return whether the operation has succeeded.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
