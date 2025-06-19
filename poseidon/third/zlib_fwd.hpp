// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_ZLIB_FWD_
#define POSEIDON_THIRD_ZLIB_FWD_

#include "../fwd.hpp"
#define ZLIB_CONST 1
#define Z_SOLO 1
#include <zlib.h>
namespace poseidon {

enum zlib_Format : uint8_t
  {
    zlib_deflate  = 0,  // deflate data with zlib header
    zlib_raw      = 1,  // raw deflate data
    zlib_gzip     = 2,  // deflate data with gzip header
  };

class scoped_deflate_stream
  {
  private:
    mutable ::z_stream m_strm[1];

  public:
    scoped_deflate_stream(zlib_Format fmt, uint8_t wbits, int level = Z_DEFAULT_COMPRESSION)
      {
        if((fmt != zlib_deflate) && (fmt != zlib_raw) && (fmt != zlib_gzip))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                "scoped_deflate_stream: format `%d` not valid", fmt);

        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                "scoped_deflate_stream: window bits `%d` not valid", wbits);

        if((level < -1) || (level > 9))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                "scoped_deflate_stream: compression level `%d` not valid", level);

        this->m_strm->zalloc = nullptr;
        this->m_strm->zfree = nullptr;
        this->m_strm->opaque = nullptr;
        this->m_strm->next_in = nullptr;
        this->m_strm->avail_in = 0;

        int fmt_wbits = wbits;
        if(fmt == zlib_raw)
          fmt_wbits *= -1;
        else if(fmt == zlib_gzip)
          fmt_wbits += 16;

        if(::deflateInit2(this->m_strm, level, Z_DEFLATED, fmt_wbits, 9,
                          Z_DEFAULT_STRATEGY) != Z_OK)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                "scoped_deflate_stream: insufficient memory");
      }

    ~scoped_deflate_stream()
      {
        ::deflateEnd(this->m_strm);
      }

    scoped_deflate_stream(const scoped_deflate_stream&) = delete;
    scoped_deflate_stream& operator=(const scoped_deflate_stream&) & = delete;

    operator ::z_stream*() const noexcept { return this->m_strm;  }

    const char*
    msg() const noexcept
      { return this->m_strm->msg ? this->m_strm->msg : "no error";  }

    void
    get_buffers(char*& ocur, const char*& icur) noexcept
      {
        ocur = reinterpret_cast<char*>(this->m_strm->next_out);
        icur = reinterpret_cast<const char*>(this->m_strm->next_in);
      }

    void
    set_buffers(char* ocur, char* oend, const char* icur, const char* iend) noexcept
      {
        this->m_strm->next_out = reinterpret_cast<::Bytef*>(ocur);
        this->m_strm->avail_out = ::rocket::clamp_cast<::uInt>(oend - ocur, 0, INT_MAX);
        this->m_strm->next_in = reinterpret_cast<const ::Bytef*>(icur);
        this->m_strm->avail_in = ::rocket::clamp_cast<::uInt>(iend - icur, 0, INT_MAX);
      }
  };

class scoped_inflate_stream
  {
  private:
    mutable ::z_stream m_strm[1];

  public:
    scoped_inflate_stream(zlib_Format fmt, uint8_t wbits)
      {
        if((fmt != zlib_deflate) && (fmt != zlib_raw) && (fmt != zlib_gzip))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                "scoped_inflate_stream: format `%d` not valid", fmt);

        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                "scoped_inflate_stream: window bits `%d` not valid", wbits);

        this->m_strm->zalloc = nullptr;
        this->m_strm->zfree = nullptr;
        this->m_strm->opaque = nullptr;
        this->m_strm->next_in = nullptr;
        this->m_strm->avail_in = 0;

        int fmt_wbits = wbits;
        if(fmt == zlib_raw)
          fmt_wbits *= -1;
        else if(fmt == zlib_gzip)
          fmt_wbits += 16;

        if(::inflateInit2(this->m_strm, fmt_wbits) != Z_OK)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                "scoped_inflate_stream: insufficient memory");
      }

    ~scoped_inflate_stream()
      {
        ::inflateEnd(this->m_strm);
      }

    scoped_inflate_stream(const scoped_inflate_stream&) = delete;
    scoped_inflate_stream& operator=(const scoped_inflate_stream&) & = delete;

    operator ::z_stream*() const noexcept { return this->m_strm;  }

    const char*
    msg() const noexcept
      { return this->m_strm->msg ? this->m_strm->msg : "no error";  }

    void
    get_buffers(char*& ocur, const char*& icur) noexcept
      {
        ocur = reinterpret_cast<char*>(this->m_strm->next_out);
        icur = reinterpret_cast<const char*>(this->m_strm->next_in);
      }

    void
    set_buffers(char* ocur, char* oend, const char* icur, const char* iend) noexcept
      {
        this->m_strm->next_out = reinterpret_cast<::Bytef*>(ocur);
        this->m_strm->avail_out = ::rocket::clamp_cast<::uInt>(oend - ocur, 0, INT_MAX);
        this->m_strm->next_in = reinterpret_cast<const ::Bytef*>(icur);
        this->m_strm->avail_in = ::rocket::clamp_cast<::uInt>(iend - icur, 0, INT_MAX);
      }
  };

}  // namespace poseidon
#endif
