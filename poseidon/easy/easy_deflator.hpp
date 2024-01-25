// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_DEFLATOR_
#define POSEIDON_EASY_EASY_DEFLATOR_

#include "../fwd.hpp"
namespace poseidon {

class Easy_Deflator
  {
  private:
    shptr<Deflator> m_defl;
    shptr<linear_buffer> m_out;

  public:
    // Constructs an empty data compressor.
    explicit
    Easy_Deflator() noexcept
      :
        m_defl(), m_out()
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_Deflator);

    // Starts a stream.
    void
    open(zlib_Format format, int level = -1);

    // Destroys the stream, freeing all allocated storage.
    void
    close() noexcept;

    // Resets the current stream. Pending data are discarded.
    void
    reset() noexcept;

    // Gets a pointer to compressed data.
    ROCKET_PURE
    const char*
    output_data() const noexcept;

    // Gets the number of bytes of compressed data.
    ROCKET_PURE
    size_t
    output_size() const noexcept;

    // Clears compressed data.
    void
    output_clear() noexcept;

    // Compresses some data and returns the number of bytes that have been
    // consumed.
    size_t
    deflate(chars_view data);

    // Completes the current deflate block.
    bool
    sync_flush();

    // Completes the current deflate block and clears state.
    bool
    full_flush();

    // Completes the current stream.
    bool
    finish();
  };

}  // namespace poseidon
#endif
