// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "inflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Inflator::
Inflator(zlib_Format format)
  :
    m_strm(format, 15)
  {
  }

Inflator::
~Inflator()
  {
  }

void
Inflator::
reset() noexcept
  {
    ::inflateReset(this->m_strm);
  }

size_t
Inflator::
inflate(chars_view data)
  {
    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;
    int err;

    do {
      constexpr size_t out_request = 128;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_inflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_inflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::inflate(this->m_strm, Z_SYNC_FLUSH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      this->do_on_inflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        this->m_strm.throw_exception(err, "inflate");
    }
    while((in_ptr != in_end) && (err == Z_OK));

    // Return the number of characters that have been consumed.
    return (size_t) (in_ptr - data.p);
  }

bool
Inflator::
finish()
  {
    const char* in_ptr = "";
    const char* in_end = in_ptr;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      constexpr size_t out_request = 16;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_inflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_inflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::inflate(this->m_strm, Z_SYNC_FLUSH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      this->do_on_inflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        this->m_strm.throw_exception(err, "inflate");
    }
    while(err == Z_OK);

    // Return whether the operation has succeeded.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
