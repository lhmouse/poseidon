// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_ZLIB_FWD_
#define POSEIDON_THIRD_ZLIB_FWD_

#include "../fwd.hpp"
#define ZLIB_CONST 1
#define Z_SOLO 1
#include <zlib.h>
namespace poseidon {

class deflate_Stream
  {
  private:
    ::z_stream m_zstrm[1];

  public:
    explicit
    deflate_Stream(zlib_Format fmt, uint8_t wbits, int level)
      {
        this->m_zstrm->zalloc = nullptr;
        this->m_zstrm->zfree = nullptr;
        this->m_zstrm->opaque = nullptr;
        this->m_zstrm->next_in = (const ::Bytef*) "";
        this->m_zstrm->avail_in = 0;

        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "deflate_Stream: window bits `%d` not valid",
              wbits);

        int fmt_wbits;
        if(fmt == zlib_deflate)
          fmt_wbits = wbits;
        else if(fmt == zlib_gzip)
          fmt_wbits = wbits + 16;
        else if(fmt == zlib_raw)
          fmt_wbits = -wbits;
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
          this->throw_exception(err, "deflateInit2");
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(deflate_Stream)
      {
        ::deflateEnd(this->m_zstrm);
      }

    [[noreturn]]
    void
    throw_exception(int err, const char* func) const
      {
        const char* msg;

        if(this->m_zstrm->msg)
          msg = this->m_zstrm->msg;
        else if(err == Z_MEM_ERROR)
          msg = "memory allocation failure";
        else if(err == Z_VERSION_ERROR)
          msg = "zlib library version mismatch";
        else
          msg = "no error message";

        ::rocket::sprintf_and_throw<::std::runtime_error>(
            "deflate_Stream: zlib error: %s\n[`%s()` returned `%d`]",
            msg, func, err);
      }

    void
    reset() noexcept
      {
        ::deflateReset(this->m_zstrm);
      }

    int
    deflate(char*& out_p, char* out_end, const char*& in_p, const char* in_end, int flush) noexcept
      {
        this->m_zstrm->next_out = (::Bytef*) out_p;
        this->m_zstrm->avail_out = (::uInt) ::rocket::min(out_end - out_p, INT_MAX);
        this->m_zstrm->next_in = (const ::Bytef*) in_p;
        this->m_zstrm->avail_in = (::uInt) ::rocket::min(in_end - in_p, INT_MAX);

        int err = ::deflate(this->m_zstrm, flush);

        out_p = (char*) this->m_zstrm->next_out;
        in_p = (const char*) this->m_zstrm->next_in;
        return err;
      }
  };

class inflate_Stream
  {
  private:
    ::z_stream m_zstrm[1];

  public:
    explicit
    inflate_Stream(zlib_Format fmt, uint8_t wbits)
      {
        this->m_zstrm->zalloc = nullptr;
        this->m_zstrm->zfree = nullptr;
        this->m_zstrm->opaque = nullptr;
        this->m_zstrm->next_in = (const ::Bytef*) "";
        this->m_zstrm->avail_in = 0;

        if((wbits < 9) || (wbits > 15))
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "inflate_Stream: window bits `%d` not valid",
              wbits);

        int fmt_wbits;
        if(fmt == zlib_deflate)
          fmt_wbits = wbits;
        else if(fmt == zlib_gzip)
          fmt_wbits = wbits + 16;
        else if(fmt == zlib_raw)
          fmt_wbits = -wbits;
        else
          ::rocket::sprintf_and_throw<::std::invalid_argument>(
              "inflate_Stream: format `%d` not valid",
              fmt);

        int err = ::inflateInit2(this->m_zstrm, fmt_wbits);
        if(err != Z_OK)
          this->throw_exception(err, "inflateInit2");
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(inflate_Stream)
      {
        ::inflateEnd(this->m_zstrm);
      }

    [[noreturn]]
    void
    throw_exception(int err, const char* func) const
      {
        const char* msg;

        if(this->m_zstrm->msg)
          msg = this->m_zstrm->msg;
        else if(err == Z_MEM_ERROR)
          msg = "memory allocation failure";
        else if(err == Z_VERSION_ERROR)
          msg = "zlib library version mismatch";
        else
          msg = "no error message";

        ::rocket::sprintf_and_throw<::std::runtime_error>(
            "inflate_Stream: zlib error: %s\n[`%s()` returned `%d`]",
            msg, func, err);
      }

    void
    reset() noexcept
      {
        ::inflateReset(this->m_zstrm);
      }

    int
    inflate(char*& out_p, char* out_end, const char*& in_p, const char* in_end) noexcept
      {
        this->m_zstrm->next_out = (::Bytef*) out_p;
        this->m_zstrm->avail_out = (::uInt) ::rocket::min(out_end - out_p, INT_MAX);
        this->m_zstrm->next_in = (const ::Bytef*) in_p;
        this->m_zstrm->avail_in = (::uInt) ::rocket::min(in_end - in_p, INT_MAX);

        int err = ::inflate(this->m_zstrm, Z_SYNC_FLUSH);

        out_p = (char*) this->m_zstrm->next_out;
        in_p = (const char*) this->m_zstrm->next_in;
        return err;
      }
  };

} // namespace poseidon
#endif
