// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_DEFLATOR_
#define POSEIDON_EASY_EASY_DEFLATOR_

#include "../fwd.hpp"
namespace poseidon {

class Easy_Deflator
  {
  private:
    shared_ptr<Deflator> m_defl;
    shared_ptr<linear_buffer> m_out;

  public:
    // Constructs an empty data compressor.
    explicit
    Easy_Deflator() = default;

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_Deflator);

    // Starts a stream. `level` shall be an integer between 0 (no
    // compression) and 9 (best compression).
    void
    start(zlib_Format format, int level = 8);

    // Clears the current stream. Pending data are discarded.
    void
    clear() noexcept;

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

    // Compresses some data and returns the number of bytes that
    // have been consumed.
    size_t
    deflate(const char* data, size_t size);

    // Completes the current deflate block.
    bool
    sync_flush();

    // Completes the current stream.
    bool
    finish();
  };

}  // namespace poseidon
#endif
