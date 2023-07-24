// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_INFLATOR_
#define POSEIDON_EASY_EASY_INFLATOR_

#include "../fwd.hpp"
namespace poseidon {

class Easy_Inflator
  {
  private:
    shptr<Inflator> m_infl;
    shptr<linear_buffer> m_out;

  public:
    // Constructs an empty data decompressor.
    explicit
    Easy_Inflator() noexcept
      : m_infl(), m_out()
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_Inflator);

    // Starts a stream.
    void
    open(zlib_Format format);

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

    // Decompresses some data and returns the number of bytes that have been
    // consumed.
    size_t
    inflate(chars_view data);

    // Completes the current stream. No data shall be written any further. If
    // no end-of-stream marker has been found in the current stream, `false` is
    // returned.
    bool
    finish();
  };

}  // namespace poseidon
#endif
