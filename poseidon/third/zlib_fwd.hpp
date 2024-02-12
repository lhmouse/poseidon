// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

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

  public:
    zlib_xStream() noexcept
      {
        this->m_zstrm->zalloc = nullptr;
        this->m_zstrm->zfree = nullptr;
        this->m_zstrm->opaque = nullptr;
        this->m_zstrm->next_in = nullptr;
        this->m_zstrm->avail_in = 0;
      }

    zlib_xStream(const zlib_xStream&) = delete;
    zlib_xStream& operator=(const zlib_xStream&) & = delete;

    operator ::z_stream*() const noexcept
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
        this->m_zstrm->avail_in = ::rocket::clamp_cast<::uInt>(iend - icur, 0, INT_MAX);
      }
  };

struct scoped_deflate_stream : zlib_xStream
  {
    scoped_deflate_stream(zlib_Format fmt, uint8_t wbits, int level)
      {
        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                      "scoped_deflate_stream: window bits `%d` not valid", wbits);

        int fmt_wbits;
        switch(fmt)
          {
          case zlib_deflate:
            fmt_wbits = wbits;
            break;

          case zlib_raw:
            fmt_wbits = -wbits;
            break;

          case zlib_gzip:
            fmt_wbits = wbits + 16;
            break;

          default:
            ::rocket::sprintf_and_throw<::std::invalid_argument>(
                        "scoped_deflate_stream: format `%d` not valid", fmt);
          }

        if((level < -1) || (level > 9))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                      "scoped_deflate_stream: compression level `%d` not valid", level);

        if(::deflateInit2(this->m_zstrm, level, Z_DEFLATED, fmt_wbits, 9,
                          Z_DEFAULT_STRATEGY) != Z_OK)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                      "scoped_deflate_stream: insufficient memory");
      }

    ~scoped_deflate_stream()
      {
        ::deflateEnd(this->m_zstrm);
      }
  };

struct scoped_inflate_stream : zlib_xStream
  {
    scoped_inflate_stream(zlib_Format fmt, uint8_t wbits)
      {
        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
                      "scoped_inflate_stream: window bits `%d` not valid", wbits);

        int fmt_wbits;
        switch(fmt)
          {
          case zlib_deflate:
            fmt_wbits = wbits;
            break;

          case zlib_raw:
            fmt_wbits = -wbits;
            break;

          case zlib_gzip:
            fmt_wbits = wbits + 16;
            break;

          default:
            ::rocket::sprintf_and_throw<::std::invalid_argument>(
                        "scoped_inflate_stream: format `%d` not valid", fmt);
          }

        int err = ::inflateInit2(this->m_zstrm, fmt_wbits);
        if(err != Z_OK)
          ::rocket::sprintf_and_throw<::std::runtime_error>(
                      "scoped_inflate_stream: insufficient memory");
      }

    ~scoped_inflate_stream()
      {
        ::inflateEnd(this->m_zstrm);
      }
  };

}  // namespace poseidon
#endif
