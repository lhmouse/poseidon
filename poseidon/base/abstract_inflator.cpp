// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "abstract_inflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Abstract_Inflator::
Abstract_Inflator(zlib_Format format)
  :
    m_strm(format, 15)
  {
  }

Abstract_Inflator::
~Abstract_Inflator()
  {
  }

void
Abstract_Inflator::
reset() noexcept
  {
    ::inflateReset(this->m_strm);
  }

size_t
Abstract_Inflator::
inflate(chars_view data)
  {
    if(data.n == 0)
      return 0;

    int err;
    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;

    do {
      constexpr size_t out_request = 128;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_inflate_resize_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "Output buffer too small (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::inflate(this->m_strm, Z_SYNC_FLUSH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      if(out_ptr != out_end)
        this->do_on_inflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        POSEIDON_THROW((
            "Failed to decompress data; zlib error: $1",
            "[`inflate()` returned `$2`]"),
            this->m_strm.msg(), err);
    }
    while((in_ptr != in_end) && (err == Z_OK));

    // Return the number of characters that have been consumed.
    return (size_t) (in_ptr - data.p);
  }

bool
Abstract_Inflator::
finish()
  {
    int err;
    const char* in_ptr = "";
    const char* in_end = in_ptr;

    do {
      // Allocate an output buffer and write compressed data there.
      constexpr size_t out_request = 16;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_inflate_resize_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "Output buffer too small (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      this->m_strm.set_buffers(out_ptr, out_end, in_ptr, in_end);
      err = ::inflate(this->m_strm, Z_SYNC_FLUSH);

      this->m_strm.get_buffers(out_ptr, in_ptr);
      if(out_ptr != out_end)
        this->do_on_inflate_truncate_output_buffer(static_cast<size_t>(out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        POSEIDON_THROW((
            "Failed to decompress data; zlib error: $1",
            "[`inflate()` returned `$2`]"),
            this->m_strm.msg(), err);
    }
    while(err == Z_OK);

    // Return whether the operation has succeeded.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
