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

POSEIDON_VISIBILITY_HIDDEN
void
Inflator::
do_check_output_buffer()
  {
    // If the output buffer is full, try allocating a larger one.
    while(this->m_strm->avail_out == 0) {
      auto outp = this->do_on_inflate_get_output_buffer();

      // Set the new output buffer.
      this->m_strm->next_out = (::Byte*) outp.first;
      this->m_strm->avail_out = clamp_cast<::uInt>(outp.second, 0, INT_MAX);
    }

    ROCKET_ASSERT(this->m_strm->next_out != nullptr);
  }

POSEIDON_VISIBILITY_HIDDEN
void
Inflator::
do_clear_pointers()
  {
    // Discard reserved but unused bytes from the output buffer.
    if(this->m_strm->avail_out != 0)
      this->do_on_inflate_truncate_output_buffer(this->m_strm->avail_out);

    this->m_strm->next_in = nullptr;
    this->m_strm->avail_in = 0;
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
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
inflate(chars_proxy data)
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = (const ::Bytef*) data.p;

    const ::Bytef* const end_in = (const ::Bytef*) data.p + data.n;
    while(this->m_strm->next_in < end_in) {
      this->m_strm->avail_in = clamp_cast<::uInt>(end_in - this->m_strm->next_in, 0, INT_MAX);

      this->do_check_output_buffer();

      int err = ::inflate(this->m_strm, Z_SYNC_FLUSH);
      if(is_none_of(err, { Z_OK, Z_STREAM_END }))
        this->m_strm.throw_exception("inflate", err);
      else if(err != Z_OK)
        break;
    }

    size_t nremaining = (size_t) (end_in - this->m_strm->next_in);
    this->do_clear_pointers();

    // Return the number of bytes that have been processed.
    return data.n - nremaining;
  }

bool
Inflator::
finish()
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = nullptr;
    this->m_strm->avail_in = 0;

    this->do_check_output_buffer();

    int err = ::inflate(this->m_strm, Z_FINISH);
    if(is_none_of(err, { Z_OK, Z_STREAM_END }))
      this->m_strm.throw_exception("inflate", err);

    this->do_clear_pointers();

    // Return whether the stream has ended.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
