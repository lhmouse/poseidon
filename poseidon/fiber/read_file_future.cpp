// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "read_file_future.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
namespace poseidon {

Read_File_Future::
Read_File_Future(cow_stringR path, int64_t offset, size_t limit)
  {
    this->m_res.path = path;
    this->m_res.offset = offset;
    this->m_res.limit = limit;
  }

Read_File_Future::
~Read_File_Future()
  {
  }

void
Read_File_Future::
do_on_abstract_future_execute()
  {
    // Open the file and get basic information.
    unique_posix_fd fd(::open(this->m_res.path.safe_c_str(), O_RDONLY | O_NOCTTY, 0));
    if(!fd)
      POSEIDON_THROW((
          "Could not open file `$1` for reading",
          "[`open()` failed: ${errno:full}]"),
          this->m_res.path);

    struct ::stat64 st;
    if(::fstat64(fd, &st) != 0)
      POSEIDON_THROW((
          "Could not get information about file `$1`",
          "[`fstat64()` failed: ${errno:full}]"),
          this->m_res.path);

    if(S_ISREG(st.st_mode) == false)
      POSEIDON_THROW((
          "Reading non-regular file `$1` not allowed"),
          this->m_res.path);

    this->m_res.accessed_on = (system_time)(seconds) st.st_atim.tv_sec + (nanoseconds) st.st_atim.tv_nsec;
    this->m_res.modified_on = (system_time)(seconds) st.st_mtim.tv_sec + (nanoseconds) st.st_mtim.tv_nsec;
    this->m_res.file_size = st.st_size;

    if(this->m_res.offset != 0) {
      // Seek to the given offset. Negative values denote offsets from the end.
      ::off_t abs_off = ::lseek64(fd, this->m_res.offset, (this->m_res.offset >= 0) ? SEEK_SET : SEEK_END);
      if(abs_off == -1)
        POSEIDON_THROW((
            "Could not reposition file `$1`",
            "[`lseek64()` failed: ${errno:full}]"),
            this->m_res.path);

      // Update `m_res.offset` to the absolute value.
      this->m_res.offset = abs_off;
    }

    while(this->m_res.data.size() < this->m_res.limit) {
      // Read bytes and append them to `m_res.data`.
      uint32_t step_limit = (uint32_t) ::std::min<size_t>(this->m_res.limit - this->m_res.data.size(), INT_MAX);
      this->m_res.data.reserve_after_end(step_limit);
      ::ssize_t step_size = POSEIDON_SYSCALL_LOOP(::read(fd, this->m_res.data.mut_end(), step_limit));
      if(step_size == 0)
        break;
      else if(step_size < 0)
        POSEIDON_THROW((
            "Could not read file `$1`",
            "[`read()` failed: ${errno:full}]"),
            this->m_res.path);

      // Accept bytes that have been read.
      this->m_res.data.accept((size_t) step_size);
    }
  }

void
Read_File_Future::
do_on_abstract_async_task_execute()
  {
    this->do_abstract_future_request();
  }

}  // namespace poseidon
