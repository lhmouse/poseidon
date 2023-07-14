// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "inflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Inflator::
Inflator(zlib_Format format)
  : m_strm(format, 15)
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
    this->m_strm.reset();
  }

size_t
Inflator::
inflate(chars_proxy data)
  {
    const char* in_ptr = data.p;
    const char* in_end = data.p + data.n;
    int err = Z_OK;

    while((in_ptr != in_end) && (err == Z_OK)) {
      constexpr size_t out_request = 128;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_inflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_inflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      err = this->m_strm.inflate(out_ptr, out_end, in_ptr, in_end);

      if(out_ptr != out_end)
        this->do_on_inflate_truncate_output_buffer((size_t) (out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        this->m_strm.throw_exception(err, "inflate");
    }

    // Return the number of characters that have been consumed.
    return (size_t) (in_ptr - data.p);
  }

bool
Inflator::
finish()
  {
    const char* in_ptr = nullptr;
    const char* in_end = nullptr;
    int err = Z_OK;

    while(err == Z_OK) {
      // Allocate an output buffer and write compressed data there.
      constexpr size_t out_request = 16;
      size_t out_size = out_request;
      char* out_ptr = this->do_on_inflate_get_output_buffer(out_size);
      if(out_size < out_request)
        POSEIDON_THROW((
            "`do_on_inflate_get_output_buffer()` shall not return smaller buffers (`$1` < `$2`)"),
            out_size, out_request);

      char* out_end = out_ptr + out_size;
      err = this->m_strm.inflate(out_ptr, out_end, in_ptr, in_end);

      if(out_ptr != out_end)
        this->do_on_inflate_truncate_output_buffer((size_t) (out_end - out_ptr));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        this->m_strm.throw_exception(err, "inflate");
    }

    // Return whether the operation has succeeded.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
