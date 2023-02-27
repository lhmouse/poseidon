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

void
Inflator::
do_inflate_prepare(const char* data)
  {
    this->m_strm->next_in = (const uint8_t*) data;
    this->m_strm->avail_in = 0;
    this->m_strm->next_out = nullptr;
    this->m_strm->avail_out = 0;
  }

int
Inflator::
do_inflate(uint8_t*& end_out, int flush)
  {
    // Set the output size...
    this->m_strm->avail_out = clamp_cast<uint32_t>(end_out - this->m_strm->next_out, 0, INT_MAX);
    if(this->m_strm->avail_out != 0)
      return ::inflate(this->m_strm, flush);

    // .. When there is no more room, ask for a new buffer.
    auto obuf = this->do_on_inflate_get_output_buffer();
    if(obuf.second == 0)
      POSEIDON_THROW((
          "Failed to allocate output buffer",
          "[`do_on_inflate_get_output_buffer()` returned null",
          "[inflator `$1` (class `$2`)]"),
          this, typeid(*this));

    this->m_strm->next_out = (uint8_t*) obuf.first;
    this->m_strm->avail_out = clamp_cast<uint32_t>(obuf.second, 0, INT_MAX);
    end_out = this->m_strm->next_out + obuf.second;

    return ::inflate(this->m_strm, flush);
  }

void
Inflator::
do_inflate_cleanup(uint8_t* end_out)
  {
    if(this->m_strm->avail_out != 0)
      this->do_on_inflate_truncate_output_buffer((size_t) (end_out - this->m_strm->next_out));

    this->m_strm->next_in = nullptr;
    this->m_strm->next_out = nullptr;
  }

size_t
Inflator::
inflate(const char* data, size_t size)
  {
    this->do_inflate_prepare(data);
    const uint8_t* end_in = (const uint8_t*) data + size;
    uint8_t* end_out = nullptr;
    int err = Z_OK;

    while(err == Z_OK)
      this->m_strm->avail_in = clamp_cast<uint32_t>(end_in - this->m_strm->next_in, 0, INT_MAX),
        err = this->do_inflate(end_out, Z_FINISH);

    if(is_none_of(err, { Z_BUF_ERROR, Z_STREAM_END }))
      this->m_strm.throw_exception("inflate", err);

    end_in = this->m_strm->next_in;
    this->do_inflate_cleanup(end_out);
    return (size_t) (end_in - (const uint8_t*) data);
  }

bool
Inflator::
finish()
  {
    this->do_inflate_prepare(nullptr);
    uint8_t* end_out = nullptr;
    int err = Z_OK;

    while(err == Z_OK)
      err = this->do_inflate(end_out, Z_FINISH);

    if(is_none_of(err, { Z_BUF_ERROR, Z_STREAM_END }))
      this->m_strm.throw_exception("inflate", err);

    this->do_inflate_cleanup(end_out);
    return err == Z_STREAM_END;
  }

}  // namespace poseidon
