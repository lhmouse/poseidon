// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "inflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Inflator::
Inflator(zlib_Options opts)
  : m_strm(opts)
  {
  }

Inflator::
~Inflator()
  {
  }

void
Inflator::
clear() noexcept
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
      // Allocate an output buffer and write compressed data there.
      auto out = this->do_on_inflate_get_output_buffer();
      char* out_end = out.first + out.second;

      err = this->m_strm.inflate(out.first, out_end, in_ptr, in_end);

      if(out.first != out_end)
        this->do_on_inflate_truncate_output_buffer((size_t) (out_end - out.first));

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
      auto out = this->do_on_inflate_get_output_buffer();
      char* out_end = out.first + out.second;

      err = this->m_strm.inflate(out.first, out_end, in_ptr, in_end);

      if(out.first != out_end)
        this->do_on_inflate_truncate_output_buffer((size_t) (out_end - out.first));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR, Z_STREAM_END }))
        this->m_strm.throw_exception(err, "inflate");
    }

    // Return whether the operation has succeeded.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
