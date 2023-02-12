// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_EASY_EASY_INFLATOR_
#define POSEIDON_EASY_EASY_INFLATOR_

#include "../fwd.hpp"
#include "../third/zlib_fwd.hpp"
namespace poseidon {

class Easy_Inflator
  {
  private:
    shared_ptr<Inflator> m_defl;

  public:
    // Constructs an empty data decompressor.
    explicit
    Easy_Inflator() = default;

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Easy_Inflator);

    // Starts a stream.
    void
    start(zlib_Format format);

    // Clears the current stream. Pending data are discarded.
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

    // Inputs some data to compress.
    void
    inflate(const char* data, size_t size);

    // Completes the current stream.  This function expects an end-of-stream
    // marker, and throws an exception if no one is encountered.
    void
    finish();
  };

}  // namespace poseidon
#endif
