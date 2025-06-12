// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "read_file_future.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
namespace poseidon {

Read_File_Future::
Read_File_Future(const cow_string& path, int64_t offset_req, size_t limit)
  {
    this->m_path = path;
    this->m_offset_req = offset_req;
    this->m_limit = limit;
  }

Read_File_Future::
~Read_File_Future()
  {
  }

void
Read_File_Future::
do_on_abstract_future_initialize()
  {
    unique_posix_fd fd(::open(this->m_path.safe_c_str(), O_RDONLY | O_NOCTTY, 0));
    if(!fd)
      POSEIDON_THROW((
          "Could not open file `$1` for reading",
          "[`open()` failed: ${errno:full}]"),
          this->m_path);

    // Get basic information.
    struct ::stat64 st;
    if(::fstat64(fd, &st) != 0)
      POSEIDON_THROW((
          "Could not get information about file `$1`",
          "[`fstat64()` failed: ${errno:full}]"),
          this->m_path);

    if(S_ISREG(st.st_mode) == false)
      POSEIDON_THROW((
          "Could not read non-regular file `$1`"),
          this->m_path);

    this->m_accessed = system_time_from_timespec(st.st_atim);
    this->m_modified = system_time_from_timespec(st.st_mtim);
    this->m_file_size = st.st_size;

    if(this->m_offset_req != 0) {
      // Seek to the requested offset. Negative values denote offsets from
      // the end of the file.
      this->m_offset = ::lseek64(fd, this->m_offset_req,
                                 (this->m_offset_req >= 0) ? SEEK_SET : SEEK_END);
      if(this->m_offset == -1)
        POSEIDON_THROW((
            "Could not reposition file `$1`",
            "[`lseek64()` failed: ${errno:full}]"),
            this->m_path);
    }

    for(;;) {
      size_t size_to_reserve = static_cast<size_t>(::rocket::min(
               static_cast<int64_t>(this->m_limit - this->m_data.size()),
               this->m_file_size - this->m_offset - this->m_data.ssize(),
               INT_MAX));  // safety value
      if(size_to_reserve == 0)
        break;

      auto base = this->m_data.insert(this->m_data.end(), size_to_reserve, '*');
      ::ssize_t ior = POSEIDON_SYSCALL_LOOP(::read(fd, &*base, size_to_reserve));
      if(ior < 0)
        POSEIDON_THROW((
            "Could not read file `$1`",
            "[`read()` failed: ${errno:full}]"),
            this->m_path);

      this->m_data.erase(base + ior, this->m_data.end());
      if(ior == 0)
        break;
    }
  }

void
Read_File_Future::
do_on_abstract_task_execute()
  {
    this->do_abstract_future_initialize_once();
  }

}  // namespace poseidon
