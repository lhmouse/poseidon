// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "read_file_future.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
namespace poseidon {

Read_File_Future::
Read_File_Future(stringR path, int64_t offset, size_t limit)
  : m_path(path), m_offset(offset), m_limit(limit)
  {
  }

Read_File_Future::
~Read_File_Future()
  {
  }

void
Read_File_Future::
do_abstract_task_on_execute()
  try {
    // Warning: This is in the worker thread. Any operation that changes data
    // members of `*this` shall be put into `do_on_future_ready()`, where `*this`
    // will have been locked.
    unique_posix_fd fd(::open(this->m_path.safe_c_str(), O_RDONLY | O_NOCTTY, 0));
    if(!fd)
      POSEIDON_THROW((
          "Could not open file `$1` for reading",
          "[`open()` failed: $2]"),
          this->m_path, format_errno());

    struct ::stat64 st;
    if(::fstat64(fd, &st) != 0)
      POSEIDON_THROW((
          "Could not get information about file `$1`",
          "[`fstat64()` failed: $2]"),
          this->m_path, format_errno());

    if(!S_ISREG(st.st_mode))
      POSEIDON_THROW((
          "Non-regular file `$1` not allowed"),
          this->m_path);

    result_type res;
    res.file_size = st.st_size;
    res.accessed_on = (system_time)(seconds) st.st_atim.tv_sec + (nanoseconds) st.st_atim.tv_nsec;
    res.modified_on = (system_time)(seconds) st.st_mtim.tv_sec + (nanoseconds) st.st_mtim.tv_nsec;
    res.offset = 0;

    if(this->m_offset != 0) {
      // If the offset is non-zero, seek there.
      ::off_t roff = ::lseek64(fd, this->m_offset, SEEK_SET);
      if(roff == (::off_t) -1)
        POSEIDON_THROW((
            "Could not reposition file `$1`",
            "[`lseek64()` failed: $2]"),
            this->m_path, format_errno());

      // This is the real offset from the beginning of the file.
      res.offset = (int64_t) roff;
    }

    for(;;) {
      uint32_t size_add = (uint32_t) min(this->m_limit - res.data.size(), 1048576U);
      if(size_add == 0)
        break;

      auto bufp = res.data.insert(res.data.end(), size_add, '*');
      ::ssize_t nread = POSEIDON_SYSCALL_LOOP(::read(fd, &*bufp, size_add));
      if(nread < 0)
        POSEIDON_THROW((
            "Could not read file `$1`",
            "[`read()` failed: $2]"),
            this->m_path, format_errno());

      res.data.erase(bufp + nread, res.data.end());
      if(nread == 0)
        break;
    }

    // Set `m_result`. This requires locking `*this` so do it in
    // `do_on_future_ready()`.
    POSEIDON_LOG_DEBUG(("File read success: path = $1, offset = $2, data.size() = $3"), this->m_path, this->m_offset, res.data.size());
    this->do_try_set_ready(&res);
  }
  catch(exception& stdex) {
    // Ensure an exception is always set, even after the first call to
    // `do_try_set_ready()` has failed.
    POSEIDON_LOG_DEBUG(("File read error: path = $1, stdex = $2"), this->m_path, stdex);
    this->do_try_set_ready(nullptr);
  }

void
Read_File_Future::
do_on_future_ready(void* param)
  {
    if(!param) {
      // Set an exception.
      this->m_except = ::std::current_exception();
      return;
    }

    this->m_result = ::std::move(*(result_type*) param);
    this->m_except = nullptr;
  }

}  // namespace poseidon
