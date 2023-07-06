// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "inflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Inflator::
Inflator(zlib_Format format, int wbits)
  : m_strm("inflateInit2",
           ::inflateInit2_,  // ... `inflateInit2()` is a macro??
           zlib_make_windowBits(wbits, format),
           ZLIB_VERSION, (int) sizeof(::z_stream))
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
    int err = ::inflateReset(this->m_strm);
    if(err != Z_OK)
      POSEIDON_LOG_ERROR((
          "zlib error ignored: $4",
          "[`inflateReset()` returned `$3`]",
          "[inflator `$1` (class `$2`)]"),
          this, typeid(*this), err, this->m_strm.message(err));
  }

size_t
Inflator::
inflate(const char* data, size_t size)
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = (const ::Bytef*) data;

    const ::Bytef* const end_in = (const ::Bytef*) data + size;
    while(this->m_strm->next_in < end_in) {
      this->m_strm->avail_in = clamp_cast<::uInt>(end_in - this->m_strm->next_in, 0, INT_MAX);

      while(this->m_strm->avail_out == 0) {
        // Extend the output buffer.
        auto outp = this->do_on_inflate_get_output_buffer();
        this->m_strm->next_out = (::Byte*) outp.first;
        this->m_strm->avail_out = clamp_cast<::uInt>(outp.second, 0, INT_MAX);
      }

      // Perform decompression now. This will never fail with `Z_BUF_ERROR`.
      int err = ::inflate(this->m_strm, Z_SYNC_FLUSH);
      if(is_none_of(err, { Z_OK, Z_STREAM_END }))
        this->m_strm.throw_exception("inflate", err);
      else if(err != Z_OK)
        break;
    }

    // Discard reserved but unused bytes from the output buffer.
    if(this->m_strm->avail_out != 0)
      this->do_on_inflate_truncate_output_buffer(this->m_strm->avail_out);

    // Return the number of bytes that have been processed.
    return size - (size_t) (end_in - this->m_strm->next_in);
  }

bool
Inflator::
finish()
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = (const ::Bytef*) "";
    this->m_strm->avail_in = 0;

    while(this->m_strm->avail_out == 0) {
      // Extend the output buffer.
      auto outp = this->do_on_inflate_get_output_buffer();
      this->m_strm->next_out = (::Byte*) outp.first;
      this->m_strm->avail_out = clamp_cast<::uInt>(outp.second, 0, INT_MAX);
    }

    // Perform compression now.
    int err = ::inflate(this->m_strm, Z_FINISH);
    if(is_none_of(err, { Z_OK, Z_STREAM_END }))
      this->m_strm.throw_exception("inflate", err);

    // Discard reserved but unused bytes from the output buffer.
    if(this->m_strm->avail_out != 0)
      this->do_on_inflate_truncate_output_buffer(this->m_strm->avail_out);

    // Return whether the stream has ended.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
