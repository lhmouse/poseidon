// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_READ_FILE_FUTURE_
#define POSEIDON_FIBER_READ_FILE_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
namespace poseidon {

class Read_File_Future
  :
    public Abstract_Future
  {
  public:
  public:
    // This is actually an input/output type.
    struct Result
      {
        cow_string path;
        int64_t offset;
        size_t limit;

        system_time accessed_on;  // time of last access
        system_time modified_on;  // time of last modification
        int64_t file_size;  // number of bytes in total
        linear_buffer data;
      };

  private:
    Result m_res;

  public:
    // Constructs a result future for reading a file. The file which `path` denotes
    // must exist, and shall be a regular file. This object also functions as an
    // asynchronous task, which can be enqueued into an `Task_Executor`. This
    // future will become ready once the read operation is complete.
    explicit Read_File_Future(const cow_string& path, int64_t offset = 0, size_t limit = INT_MAX);

  private:
    virtual
    void
    do_on_abstract_future_execute() override;

  public:
    Read_File_Future(const Read_File_Future&) = delete;
    Read_File_Future& operator=(const Read_File_Future&) & = delete;
    virtual ~Read_File_Future();

    // Gets the result if `successful()` yields `true`. If `successful()` yields
    // `false`, an exception is thrown, and there is no effect.
    const Result&
    result() const
      {
        this->check_success();
        return this->m_res;
      }

    Result&
    mut_result()
      {
        this->check_success();
        return this->m_res;
      }
  };

}  // namespace poseidon
#endif
