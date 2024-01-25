// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_WEBSOCKET_DEFLATOR_
#define POSEIDON_HTTP_WEBSOCKET_DEFLATOR_

#include "../fwd.hpp"
#include "../third/zlib_fwd.hpp"
namespace poseidon {

class WebSocket_Deflator
  {
  private:
    // deflator (send)
    mutable plain_mutex m_def_mtx;
    deflate_Stream m_def_strm;
    linear_buffer m_def_buf;
    cacheline_barrier m_xcb_1;

    // inflator (recv)
    mutable plain_mutex m_inf_mtx;
    inflate_Stream m_inf_strm;
    linear_buffer m_inf_buf;

  public:
    // Initializes a new deflator/inflator with PMCE arguments from `parser`. A
    // previous WebSocket handshake shall have completed.
    explicit WebSocket_Deflator(const WebSocket_Frame_Parser& parser);

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(WebSocket_Deflator);

    // Get the deflator output buffer.
    linear_buffer&
    deflate_output_buffer(plain_mutex::unique_lock& lock) noexcept
      {
        lock.lock(this->m_def_mtx);
        return this->m_def_buf;
      }

    // Resets the deflator state. This is used when `no_context_takeover` is in
    // effect.
    void
    deflate_reset(plain_mutex::unique_lock& lock) noexcept;

    // Compresses a part of the frame payload.
    void
    deflate_message_stream(plain_mutex::unique_lock& lock, chars_view data);

    // Completes this frame. This function flushes all pending output, removes
    // the final `00 00 FF FF`.
    void
    deflate_message_finish(plain_mutex::unique_lock& lock);

    // Get the inflator output buffer.
    linear_buffer&
    inflate_output_buffer(plain_mutex::unique_lock& lock) noexcept
      {
        lock.lock(this->m_inf_mtx);
        return this->m_inf_buf;
      }

    // Decompresses a part of the frame payload.
    void
    inflate_message_stream(plain_mutex::unique_lock& lock, chars_view data);

    // Completes this frame. This function appends a final `00 00 FF FF`, flushes
    // all pending output.
    void
    inflate_message_finish(plain_mutex::unique_lock& lock);
  };

}  // namespace poseidon
#endif
