// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_ZLIB_FWD_
#define POSEIDON_THIRD_ZLIB_FWD_

#include "../fwd.hpp"
#define ZLIB_CONST 1
#define Z_SOLO 1
#include <zlib.h>
namespace poseidon {

class zlib_xStream
  {
  protected:
    mutable ::z_stream m_zstrm[1];

  protected:
    zlib_xStream() noexcept
      {
        this->m_zstrm->zalloc = nullptr;
        this->m_zstrm->zfree = nullptr;
        this->m_zstrm->opaque = nullptr;
        this->m_zstrm->next_in = nullptr;
        this->m_zstrm->avail_in = 0;
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(zlib_xStream) = default;

  public:
    operator
    ::z_stream*() const noexcept
      { return this->m_zstrm;  }

    const char*
    msg() const noexcept
      { return this->m_zstrm->msg ? this->m_zstrm->msg : "no error";  }

    void
    get_buffers(char*& ocur, const char*& icur) noexcept
      {
        ocur = reinterpret_cast<char*>(this->m_zstrm->next_out);
        icur = reinterpret_cast<const char*>(this->m_zstrm->next_in);
      }

    void
    set_buffers(char* ocur, char* oend, const char* icur, const char* iend) noexcept
      {
        this->m_zstrm->next_out = reinterpret_cast<::Bytef*>(ocur);
        this->m_zstrm->avail_out = ::rocket::clamp_cast<::uInt>(oend - ocur, 0, INT_MAX);
        this->m_zstrm->next_in = reinterpret_cast<const ::Bytef*>(icur);
        this->m_zstrm->avail_in = ::rocket::clamp_cast<const ::uInt>(iend - icur, 0, INT_MAX);
      }
  };

struct deflate_Stream : zlib_xStream
  {
    explicit
    deflate_Stream(zlib_Format fmt, uint8_t wbits, int level)
      {
        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "deflate_Stream: window bits `%d` not valid",
              wbits);

        int fmt_wbits;
        if(fmt == zlib_deflate)
          fmt_wbits = wbits;
        else if(fmt == zlib_raw)
          fmt_wbits = -wbits;
        else if(fmt == zlib_gzip)
          fmt_wbits = wbits + 16;
        else
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "deflate_Stream: format `%d` not valid",
              fmt);

        if((level < -1) || (level > 9))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "deflate_Stream: compression level `%d` not valid",
              level);

        int err = ::deflateInit2(this->m_zstrm, level, Z_DEFLATED, fmt_wbits, 9, Z_DEFAULT_STRATEGY);
        if(err != Z_OK)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                "deflate_Stream: insufficient memory");
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(deflate_Stream)
      {
        ::deflateEnd(this->m_zstrm);
      }
  };

struct inflate_Stream : zlib_xStream
  {
    explicit
    inflate_Stream(zlib_Format fmt, uint8_t wbits)
      {
        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "inflate_Stream: window bits `%d` not valid",
              wbits);

        int fmt_wbits;
        if(fmt == zlib_deflate)
          fmt_wbits = wbits;
        else if(fmt == zlib_raw)
          fmt_wbits = -wbits;
        else if(fmt == zlib_gzip)
          fmt_wbits = wbits + 16;
        else
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "inflate_Stream: format `%d` not valid",
              fmt);

        int err = ::inflateInit2(this->m_zstrm, fmt_wbits);
        if(err != Z_OK)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                "sprintf_and_throw: insufficient memory");
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(inflate_Stream)
      {
        ::inflateEnd(this->m_zstrm);
      }
  };

}  // namespace poseidon
#endif
