// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_ASYNC_LOGGER_
#define POSEIDON_STATIC_ASYNC_LOGGER_

#include "../fwd.hpp"
namespace poseidon {

// This class buffers log messages and write them asynchronously.
// Objects of this class are recommended to be static.
class Async_Logger
  {
  private:
    struct X_Level_Config;

    mutable plain_mutex m_conf_mutex;
    cow_vector<X_Level_Config> m_conf_levels;
    atomic_relaxed<uint32_t> m_conf_level_bits;

    mutable plain_mutex m_queue_mutex;
    condition_variable m_queue_avail;
    vector<Log_Message> m_queue;

    mutable recursive_mutex m_io_mutex;
    vector<Log_Message> m_io_queue;

  public:
    // Creates a logger that outputs to nowhere.
    explicit
    Async_Logger();

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Async_Logger);

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& file);

    // Performs I/O operation.
    // This function should be called by the logger thread repeatedly.
    void
    thread_loop();

    // Checks whether a given level is enabled.
    // This function is thread-safe.
    ROCKET_PURE
    bool
    level_enabled(Log_Level level) const noexcept
      {
        return (level <= 15U) && (this->m_conf_level_bits.load() >> level & 1U);
      }

    // Enqueues a log message.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    enqueue(Log_Message&& msg);

    // Waits until all pending log entries are delivered to output devices.
    // This function is thread-safe.
    void
    synchronize() noexcept;
  };

}  // namespace poseidon
#endif
