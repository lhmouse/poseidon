// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "websocket_deflator.hpp"
#include "websocket_frame_parser.hpp"
#include "../utils.hpp"
namespace poseidon {

WebSocket_Deflator::
WebSocket_Deflator(const WebSocket_Frame_Parser& parser)
  : m_def_strm(zlib_raw, parser.pmce_send_max_window_bits(),  parser.pmce_send_compression_level()),
    m_inf_strm(zlib_raw, parser.pmce_recv_max_window_bits())
  {
  }

WebSocket_Deflator::
~WebSocket_Deflator()
  {
  }

void
WebSocket_Deflator::
deflate_message_stream(plain_mutex::unique_lock& lock, chars_proxy data)
  {
    lock.lock(this->m_def_mtx);

    if(data.n == 0)
      return;

    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_def_buf.reserve_after_end(1024);
      char* out_ptr = this->m_def_buf.mut_end();

      err = this->m_def_strm.deflate(out_ptr, out_ptr + out_size, in_ptr, in_end, Z_NO_FLUSH);
      this->m_def_buf.accept((size_t) (out_ptr - this->m_def_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        this->m_def_strm.throw_exception(err, "deflate");
    }
    while((in_ptr != in_end) && (err == Z_OK));
  }

void
WebSocket_Deflator::
deflate_message_finish(plain_mutex::unique_lock& lock)
  {
    lock.lock(this->m_def_mtx);

    const char* in_ptr = "";
    const char* in_end = in_ptr;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_def_buf.reserve_after_end(16);
      char* out_ptr = this->m_def_buf.mut_end();

      err = this->m_def_strm.deflate(out_ptr, out_ptr + out_size, in_ptr, in_end, Z_SYNC_FLUSH);
      this->m_def_buf.accept((size_t) (out_ptr - this->m_def_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        this->m_def_strm.throw_exception(err, "deflate");
    }
    while(err == Z_OK);

    if((this->m_def_buf.size() >= 4) && xmemeq(this->m_def_buf.end() - 4, "\x00\x00\xFF\xFF", 4))
      this->m_def_buf.unaccept(4);
  }

void
WebSocket_Deflator::
inflate_message_stream(plain_mutex::unique_lock& lock, chars_proxy data)
  {
    lock.lock(this->m_inf_mtx);

    if(data.n == 0)
      return;

    const char* in_ptr = data.p;
    const char* in_end = in_ptr + data.n;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_inf_buf.reserve_after_end(1024);
      char* out_ptr = this->m_inf_buf.mut_end();

      err = this->m_inf_strm.inflate(out_ptr, out_ptr + out_size, in_ptr, in_end);
      this->m_inf_buf.accept((size_t) (out_ptr - this->m_inf_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        this->m_inf_strm.throw_exception(err, "inflate");
    }
    while((in_ptr != in_end) && (err == Z_OK));
  }

void
WebSocket_Deflator::
inflate_message_finish(plain_mutex::unique_lock& lock)
  {
    lock.lock(this->m_inf_mtx);

    const char* in_ptr = "\x00\x00\xFF\xFF";
    const char* in_end = in_ptr + 4;
    int err;

    do {
      // Allocate an output buffer and write compressed data there.
      size_t out_size = this->m_inf_buf.reserve_after_end(16);
      char* out_ptr = this->m_inf_buf.mut_end();

      err = this->m_inf_strm.inflate(out_ptr, out_ptr + out_size, in_ptr, in_end);
      this->m_inf_buf.accept((size_t) (out_ptr - this->m_inf_buf.mut_end()));

      if(is_none_of(err, { Z_OK, Z_BUF_ERROR }))
        this->m_inf_strm.throw_exception(err, "inflate");
    }
    while(err == Z_OK);
  }

}  // namespace poseidon
