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
      case zlib_format_deflate:
        return wbits;

      case zlib_format_raw:
        return - wbits;

      case zlib_format_gzip:
        return 16 + wbits;

      default:
        ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "zlib error: format `%d` not valid",
            format);
    }
  }

template<int cleanupT(::z_stream*)>
class zlib_Stream
  {
  private:
    ::z_stream m_strm[1];

  public:
    // Initializes a stream with `initializer`, which will be destroyed with
    // `cleanupT`. If `initializer` and `cleanupT()` do not match, the behavior
    // is undefined.
    template<typename InitializerT, typename... ParamsT>
    zlib_Stream(const char* func, InitializerT&& initializer, ParamsT&&... params)
      {
        this->m_strm->zalloc = nullptr;
        this->m_strm->zfree = nullptr;
        this->m_strm->opaque = nullptr;
        this->m_strm->msg = nullptr;

        int err = initializer(this->m_strm, ::std::forward<ParamsT>(params)...);
        if(err != Z_OK)
          this->throw_exception(func, err);
      }

    ~zlib_Stream()
      {
        cleanupT(this->m_strm);
      }

    zlib_Stream(const zlib_Stream&) = delete;
    zlib_Stream& operator=(const zlib_Stream&) = delete;

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

        return "no error message available";
      }

    [[noreturn]]
    void
    throw_exception(const char* func, int err) const
      {
        ::rocket::sprintf_and_throw<::std::runtime_error>(
            "zlib error: `%s()` returned `%d`: %s",
            func, err, this->message(err));
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

using zlib_Deflate_Stream = zlib_Stream<::deflateEnd>;
using zlib_Inflate_Stream = zlib_Stream<::inflateEnd>;

} // namespace poseidon
#endif
