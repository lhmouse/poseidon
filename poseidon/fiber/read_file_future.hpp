// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_READ_FILE_FUTURE_
#define POSEIDON_FIBER_READ_FILE_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_task.hpp"
namespace poseidon {

class Read_File_Future
  :
    public Abstract_Future,
    public Abstract_Task
  {
  private:
    cow_string m_path;
    int64_t m_offset_req;
    size_t m_limit;

    system_time m_accessed;
    system_time m_modified;
    int64_t m_file_size = -1;
    int64_t m_offset = 0;
    cow_string m_data;

  public:
    // Constructs a result future for reading a file. The file which `path`
    // denotes must exist, and must be a regular file. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Scheduler`.
    // This future will become ready once the read operation is complete.
    explicit
    Read_File_Future(const cow_string& path, int64_t offset_req = 0, size_t limit = INT_MAX);

  private:
    virtual
    void
    do_on_abstract_future_initialize() override;

    virtual
    void
    do_on_abstract_task_execute() override;

  public:
    Read_File_Future(const Read_File_Future&) = delete;
    Read_File_Future& operator=(const Read_File_Future&) & = delete;
    virtual ~Read_File_Future();

    // Gets the path to the file. This field is set by the constructor.
    const cow_string&
    path() const noexcept
      { return this->m_path;  }

    // Gets the file offset to start from. This field is set by the constructor.
    int64_t
    offset_req() const noexcept
      { return this->m_offset_req;  }

    // Gets the max number of bytes to read. This field is set by the constructor.
    size_t
    limit() const noexcept
      { return this->m_limit;  }

    // Gets the time of last access after the operation has completed successfully.
    // If `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    system_time
    time_accessed() const
      {
        this->check_success();
        return this->m_accessed;
      }

    // Gets the time of last modification after the operation has completed
    // successfully. If `successful()` yields `false`, an exception is thrown, and
    // there is no effect.
    system_time
    time_modified() const
      {
        this->check_success();
        return this->m_modified;
      }

    // Gets the file size in total after the operation has completed successfully.
    // If `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    int64_t
    file_size() const
      {
        this->check_success();
        return this->m_file_size;
      }

    // Gets the absolute file offset from the beginning of after the operation has
    // completed successfully. If `successful()` yields `false`, an exception is
    // thrown, and there is no effect.
    int64_t
    offset() const
      {
        this->check_success();
        return this->m_offset;
      }

    // Gets result data after the operation has completed successfully, which may
    // be fewer than `limit()`. If `successful()` yields `false`, an exception is
    // thrown, and there is no effect.
    const cow_string&
    data() const
      {
        this->check_success();
        return this->m_data;
      }
  };

}  // namespace poseidon
#endif
