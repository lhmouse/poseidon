// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "deflator.hpp"
#include "../utils.hpp"
namespace poseidon {

Deflator::
Deflator(zlib_Format format, int level, int wbits)
  : m_strm("deflateInit2",
           ::deflateInit2_,  // ... `deflateInit2()` is a macro??
           zlib_make_level(level),
           Z_DEFLATED,
           zlib_make_windowBits(wbits, format),
           9, Z_DEFAULT_STRATEGY,
           ZLIB_VERSION, (int) sizeof(::z_stream))
  {
  }

Deflator::
~Deflator()
  {
  }

POSEIDON_VISIBILITY_HIDDEN
void
Deflator::
do_check_output_buffer()
  {
    // If the output buffer is full, try allocating a larger one.
    while(this->m_strm->avail_out == 0) {
      auto outp = this->do_on_deflate_get_output_buffer();

      // Set the new output buffer.
      this->m_strm->next_out = (::Byte*) outp.first;
      this->m_strm->avail_out = clamp_cast<::uInt>(outp.second, 0, INT_MAX);
    }

    if(this->m_strm->avail_out < 12)
      POSEIDON_LOG_ERROR((
          "zlib output buffer too small; behavior may be unpredictable",
          "[deflator `$1` (class `$2`)]"),
          this, typeid(*this));

    ROCKET_ASSERT(this->m_strm->next_out != nullptr);
  }

POSEIDON_VISIBILITY_HIDDEN
void
Deflator::
do_clear_pointers()
  {
    // Discard reserved but unused bytes from the output buffer.
    if(this->m_strm->avail_out != 0)
      this->do_on_deflate_truncate_output_buffer(this->m_strm->avail_out);

    this->m_strm->next_in = nullptr;
    this->m_strm->avail_in = 0;
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
  }

void
Deflator::
clear() noexcept
  {
    int err = ::deflateReset(this->m_strm);
    if(err != Z_OK)
      POSEIDON_LOG_ERROR((
          "zlib error ignored: $4",
          "[`deflateReset()` returned `$3`]",
          "[deflator `$1` (class `$2`)]"),
          this, typeid(*this), err, this->m_strm.message(err));
  }

size_t
Deflator::
deflate(const char* data, size_t size)
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = (const ::Bytef*) data;

    const ::Bytef* const end_in = (const ::Bytef*) data + size;
    while(this->m_strm->next_in < end_in) {
      this->m_strm->avail_in = clamp_cast<::uInt>(end_in - this->m_strm->next_in, 0, INT_MAX);

      this->do_check_output_buffer();

      int err = ::deflate(this->m_strm, Z_NO_FLUSH);
      if(is_none_of(err, { Z_OK, Z_STREAM_END, Z_STREAM_ERROR }))
        this->m_strm.throw_exception("deflate", err);
      else if(err != Z_OK)
        break;
    }

    size_t nremaining = (size_t) (end_in - this->m_strm->next_in);
    this->do_clear_pointers();

    // Return the number of bytes that have been processed.
    return size - nremaining;
  }

bool
Deflator::
sync_flush()
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = nullptr;
    this->m_strm->avail_in = 0;

    this->do_check_output_buffer();

    int err = ::deflate(this->m_strm, Z_SYNC_FLUSH);
    if(is_none_of(err, { Z_OK, Z_STREAM_ERROR, Z_BUF_ERROR }))
      this->m_strm.throw_exception("deflate", err);

    this->do_clear_pointers();

    // Return whether something has been written.
    return err != Z_BUF_ERROR;
  }

bool
Deflator::
full_flush()
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = nullptr;
    this->m_strm->avail_in = 0;

    this->do_check_output_buffer();

    int err = ::deflate(this->m_strm, Z_FULL_FLUSH);
    if(is_none_of(err, { Z_OK, Z_STREAM_ERROR, Z_BUF_ERROR }))
      this->m_strm.throw_exception("deflate", err);

    this->do_clear_pointers();

    // Return whether something has been written.
    return err != Z_BUF_ERROR;
  }

bool
Deflator::
finish()
  {
    // Set up the output and input buffers.
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
    this->m_strm->next_in = nullptr;
    this->m_strm->avail_in = 0;

    this->do_check_output_buffer();

    int err = ::deflate(this->m_strm, Z_FINISH);
    if(is_none_of(err, { Z_OK, Z_STREAM_END }))
      this->m_strm.throw_exception("deflate", err);

    this->do_clear_pointers();

    // Return whether the stream has ended.
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
