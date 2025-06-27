// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_BASE_APPOINTMENT_
#define POSEIDON_BASE_APPOINTMENT_

#include "../fwd.hpp"
namespace poseidon {

class Appointment
  {
  private:
    ::rocket::unique_posix_fd m_fd;
    int m_index = -1;

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

    // Gets the 0-based index of the current appointment. If this appointment
    // is empty, -1 is returned.
    int
    index() const noexcept
      { return this->m_index;  }

    // Locks the file that is denoted by `path`. If the file can't be locked,
    // an exception is thrown, and no file is locked.
    void
    enroll(const cow_string& lock_path);

    // Unlocks the current file and resets the index to -1.
    void
    withdraw() noexcept;
  };

}  // namespace poseidon
#endif
