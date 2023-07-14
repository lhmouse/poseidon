// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_READ_FILE_FUTURE_
#define POSEIDON_FIBER_READ_FILE_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_async_task.hpp"
namespace poseidon {

class Read_File_Future
  : public Abstract_Future,
    public Abstract_Async_Task
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
    Result m_result;

  public:
    // Constructs a result future for reading a file. The file which `path` denotes
    // must exist, and shall be a regular file. This object also functions as an
    // asynchronous task, which can be enqueued into an `Async_Task_Executor`. This
    // future will become ready once the read operation is complete.
    explicit
    Read_File_Future(cow_stringR path, int64_t offset = 0, size_t limit = INT_MAX);

  private:
    // Performs the read operation.
    virtual
    void
    do_abstract_task_on_execute() override;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Read_File_Future);

    const Result&
    result() const
      {
        this->check_ready();
        return this->m_result;
      }

    Result&
    mut_result()
      {
        this->check_ready();
        return this->m_result;
      }
  };

}  // namespace poseidon
#endif
