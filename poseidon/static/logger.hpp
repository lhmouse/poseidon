// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_LOGGER_
#define POSEIDON_STATIC_LOGGER_

#include "../fwd.hpp"
namespace poseidon {

class Logger
  {
  private:
    mutable plain_mutex m_conf_mutex;
    struct X_Level_Config;
    cow_vector<X_Level_Config> m_conf_levels;
    atomic_relaxed<uint32_t> m_conf_level_bits;

    mutable plain_mutex m_queue_mutex;
    condition_variable m_queue_avail;
    struct X_Message;
    vector<X_Message> m_queue;

    mutable recursive_mutex m_io_mutex;
    vector<X_Message> m_io_queue;

  public:
    // Creates a logger that outputs to nowhere.
    Logger() noexcept;

  public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) & = delete;
    ~Logger();

    // Reloads configuration from 'main.conf'.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    reload(const Config_File& conf_file, bool verbose);

    // Performs I/O operation.
    // This function should be called by the logger thread repeatedly.
    void
    thread_loop();

    // Checks whether a given level is enabled.
    // This function is thread-safe.
    ROCKET_PURE
    bool
    enabled(uint8_t level) const noexcept
      {
        return (level <= 15U) && (this->m_conf_level_bits.load() & (1U << level));
      }

    // Enqueues a log message.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    enqueue(uint8_t level, const char* func, const char* file, uint32_t line, cow_stringR text);

    // Waits until all pending log entries are delivered to output devices.
    // This function is thread-safe.
    void
    synchronize() noexcept;
  };

}  // namespace poseidon
#endif
