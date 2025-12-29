// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "websocket_deflator.hpp"
#include "websocket_frame_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

WebSocket_Deflator::
WebSocket_Deflator(const WebSocket_Frame_Parser& parser)
  :
    m_def_strm(zlib_raw, parser.pmce_send_window_bits(), parser.pmce_compression_level()),
    m_inf_strm(zlib_raw, parser.pmce_receive_window_bits())
  {
  }

WebSocket_Deflator::
~WebSocket_Deflator()
  {
  }

void
WebSocket_Deflator::
deflate_reset(plain_mutex::unique_lock& lock)
  noexcept
  {
    lock.lock(this->m_def_mtx);
    ::deflateReset(this->m_def_strm);
  }

void
WebSocket_Deflator::
deflate_message_stream(plain_mutex::unique_lock& lock, chars_view data)
  {
    if(data.n == 0)
      return;

    lock.lock(this->m_def_mtx);
    int err;
    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_def_buf.reserve_after_end(1024);
      char* out_ptr = this->m_def_buf.mut_end();
      this->m_def_strm.set_buffers(out_ptr, out_ptr + out_size, in_ptr, in_end);
      err = ::deflate(this->m_def_strm, Z_NO_FLUSH);

      this->m_def_strm.get_buffers(out_ptr, in_ptr);
      this->m_def_buf.accept(static_cast<size_t>(out_ptr - this->m_def_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        POSEIDON_THROW((
            "Failed to compress WebSocket message; zlib error: $1",
            "[`deflate()` returned `$2`]"),
            this->m_def_strm.msg(), err);
    } while((in_ptr != in_end) && (err == Z_OK));
  }

void
WebSocket_Deflator::
deflate_message_finish(plain_mutex::unique_lock& lock)
  {
    lock.lock(this->m_def_mtx);
    int err;
    const char* in_ptr = "";
    const char* in_end = in_ptr;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_def_buf.reserve_after_end(16);
      char* out_ptr = this->m_def_buf.mut_end();
      this->m_def_strm.set_buffers(out_ptr, out_ptr + out_size, in_ptr, in_end);
      err = ::deflate(this->m_def_strm, Z_SYNC_FLUSH);

      this->m_def_strm.get_buffers(out_ptr, in_ptr);
      this->m_def_buf.accept(static_cast<size_t>(out_ptr - this->m_def_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        POSEIDON_THROW((
            "Failed to compress WebSocket message; zlib error: $1",
            "[`deflate()` returned `$2`]"),
            this->m_def_strm.msg(), err);
    } while(err == Z_OK);

    if((this->m_def_buf.size() >= 4)
       && (::memcmp(this->m_def_buf.end() - 4, "\x00\x00\xFF\xFF", 4) == 0))
      this->m_def_buf.unaccept(4);
  }

void
WebSocket_Deflator::
inflate_message_stream(plain_mutex::unique_lock& lock, chars_view data,
                       uint32_t max_message_length)
  {
    if(data.n == 0)
      return;

    lock.lock(this->m_inf_mtx);
    int err;
    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_inf_buf.reserve_after_end(1024);
      char* out_ptr = this->m_inf_buf.mut_end();
      this->m_inf_strm.set_buffers(out_ptr, out_ptr + out_size, in_ptr, in_end);
      err = ::inflate(this->m_inf_strm, Z_SYNC_FLUSH);

      this->m_inf_strm.get_buffers(out_ptr, in_ptr);
      this->m_inf_buf.accept(static_cast<size_t>(out_ptr - this->m_inf_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        POSEIDON_THROW((
            "Failed to decompress WebSocket message; zlib error: $1",
            "[`inflate()` returned `$2`]"),
            this->m_inf_strm.msg(), err);

      if(this->m_inf_buf.size() > max_message_length)
        POSEIDON_THROW((
            "WebSocket message length limit exceeded: `$1` > `$2`"),
            this->m_inf_buf.size(), max_message_length);
    } while((in_ptr != in_end) && (err == Z_OK));
  }

void
WebSocket_Deflator::
inflate_message_finish(plain_mutex::unique_lock& lock)
  {
    lock.lock(this->m_inf_mtx);
    int err;
    const char* in_ptr = "\x00\x00\xFF\xFF";
    const char* in_end = in_ptr + 4;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_inf_buf.reserve_after_end(16);
      char* out_ptr = this->m_inf_buf.mut_end();
      this->m_inf_strm.set_buffers(out_ptr, out_ptr + out_size, in_ptr, in_end);
      err = ::inflate(this->m_inf_strm, Z_SYNC_FLUSH);

      this->m_inf_strm.get_buffers(out_ptr, in_ptr);
      this->m_inf_buf.accept(static_cast<size_t>(out_ptr - this->m_inf_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        POSEIDON_THROW((
            "Failed to decompress WebSocket message; zlib error: $1",
            "[`inflate()` returned `$2`]"),
            this->m_inf_strm.msg(), err);
    } while(err == Z_OK);
  }

}  // namespace poseidon
