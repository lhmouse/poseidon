// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_THIRD_ZLIB_FWD_
#define POSEIDON_THIRD_ZLIB_FWD_

#include "../fwd.hpp"
#define ZLIB_CONST 1
#define Z_SOLO 1
#include <zlib.h>
namespace poseidon {

inline
int
zlib_make_level(int level)
  {
    if(level == -1)
      return Z_DEFAULT_COMPRESSION;

    if((level < 0) || (level > 9))
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "zlib error: compression level `%d` not valid",
          level);

    return level;
  }

inline
int
zlib_make_windowBits(int wbits, zlib_Format format)
  {
    if((wbits < 9) || (wbits > 15))
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
          "zlib error: window bits `%d` not valid",
          wbits);

    switch(format) {
      case zlib_deflate:
        return wbits;

      case zlib_raw:
        return - wbits;

      case zlib_gzip:
        return 16 + wbits;

      default:
        ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "zlib error: format `%d` not valid",
            format);
    }
  }

template<int xEndT(::z_stream*)>
class zlib_Stream
  {
  private:
    ::z_stream m_strm[1];

  public:
    // Initializes a stream with `xInit`, which will be destroyed with `xEndT`.
    // If `xInit` and `xEndT` do not match, the behavior is undefined.
    template<typename xInitT, typename... xArgsT>
    zlib_Stream(const char* func, xInitT&& xInit, xArgsT&&... xargs)
      {
        this->m_strm->zalloc = nullptr;
        this->m_strm->zfree = nullptr;
        this->m_strm->opaque = nullptr;

        this->m_strm->msg = nullptr;
        this->m_strm->next_in = (const ::Bytef*) "";
        this->m_strm->avail_in = 0;

        int err = xInit(this->m_strm, ::std::forward<xArgsT>(xargs)...);
        if(err != Z_OK)
          this->throw_exception(func, err);
      }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(zlib_Stream)
      {
        xEndT(this->m_strm);
      }

  public:
    // Converts zlib error codes.
    const char*
    message(int err) const
      {
        if(m_strm->msg)
          return m_strm->msg;

        if(err == Z_VERSION_ERROR)
          return "zlib library version mismatch";

        if(err == Z_MEM_ERROR)
          return "memory allocation failure";

        return "no error message";
      }

    [[noreturn]]
    void
    throw_exception(const char* func, int err) const
      {
        ::rocket::sprintf_and_throw<::std::runtime_error>(
            "zlib error: %s\n[`%s()` returned `%d`]",
            this->message(err), func, err);
      }

    operator
    const ::z_stream*() const noexcept
      { return this->m_strm;  }

    operator
    ::z_stream*() noexcept
      { return this->m_strm;  }

    const ::z_stream&
    operator*() const noexcept
      { return *(this->m_strm);  }

    ::z_stream&
    operator*() noexcept
      { return *(this->m_strm);  }

    const ::z_stream*
    operator->() const noexcept
      { return this->m_strm;  }

    ::z_stream*
    operator->() noexcept
      { return this->m_strm;  }
  };

using deflate_Stream = zlib_Stream<::deflateEnd>;
using inflate_Stream = zlib_Stream<::inflateEnd>;

} // namespace poseidon
#endif
