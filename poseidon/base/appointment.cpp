// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "appointment.hpp"
#include "../utils.hpp"
namespace poseidon {

Appointment::
Appointment(const cow_string& lock_path)
  {
    this->enroll(lock_path);
  }

Appointment::
~Appointment()
  {
  }

void
Appointment::
enroll(const cow_string& lock_path)
  {
    this->m_fd.reset();
    this->m_index = -1;

    // Create the lock file.
    ::rocket::unique_handle<int, closer> fd;
    fd.reset(::open(lock_path.safe_c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600));
    if(fd == -1)
      POSEIDON_THROW((
          "Could not create or open file '$1'",
          "[`open()` failed: ${errno:full}]"),
          lock_path);

    // Get my serial number.
    struct flock lck = { };
    lck.l_type = F_WRLCK;
    lck.l_len = 1;
    while(::fcntl(fd, F_OFD_SETLK, &lck) != 0)
      if(errno == EAGAIN)
        lck.l_start ++;
      else
        POSEIDON_THROW((
            "Could not lock file '$1'",
            "[`fcntl()` failed: ${errno:full}]"),
            lock_path);

    // Adopt the lock.
    this->m_fd = move(fd);
    this->m_index = static_cast<int>(lck.l_start);
  }

void
Appointment::
withdraw() noexcept
  {
    this->m_fd.reset();
    this->m_index = -1;
  }

}  // namespace poseidon
