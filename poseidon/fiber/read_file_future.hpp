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
    struct result_type
      {
        int64_t file_size;  // number of bytes in total
        system_time accessed_on;  // time of last access
        system_time modified_on;  // time of last modification
        int64_t offset;  // data offset
        cow_string data;
      };

  private:
    // read-only
    cow_string m_path;
    int64_t m_offset;
    size_t m_limit;

    // result
    result_type m_result;
    exception_ptr m_except;

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

    // Sets the result.
    virtual
    void
    do_on_future_ready(void* param) override;

  public:
    ASTERIA_NONCOPYABLE_VIRTUAL_DESTRUCTOR(Read_File_Future);

    // Gets the result.
    const result_type&
    result() const
      {
        if(!this->ready())
          ::rocket::sprintf_and_throw<::std::runtime_error>(
              "Read_File_Future: File operation in progress");

        if(this->m_except)
          ::std::rethrow_exception(this->m_except);

        return this->m_result;
      }
  };

}  // namespace poseidon
#endif
