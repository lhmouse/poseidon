// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_APPOINTMENT_
#define POSEIDON_BASE_APPOINTMENT_

#include "../fwd.hpp"
namespace poseidon {

class Appointment
  {
  private:
    struct closer
      {
        constexpr int null() const noexcept { return -1;  }
        constexpr bool is_null(int fd) const noexcept { return fd == -1;  }
        void close(int fd) noexcept { ::close(fd);  }
      };

    ::rocket::unique_handle<int, closer> m_fd;
    uint32_t m_serial = 0;

  public:
    // Constructs an empty appointment.
    constexpr
    Appointment() noexcept = default;

    // Locks the file that is denoted by `lock_path`. This has the same effect
    // as `enroll(lock_path)`.
    explicit
    Appointment(const cow_string& lock_path);

  public:
    Appointment(const Appointment&) = delete;
    Appointment& operator=(const Appointment&) & = delete;
    ~Appointment();

    // Gets the 1-based serial number of the current appointment. If this
    // appointment is empty, zero is returned.
    uint32_t
    serial() const noexcept
      { return this->m_serial;  }

    // Locks the file that is denoted by `path`. If the file can't be locked,
    // an exception is thrown, and no file is locked.
    void
    enroll(const cow_string& lock_path);

    // Unlocks the current file and resets the serial to zero.
    void
    withdraw() noexcept;
  };

}  // namespace poseidon
#endif
