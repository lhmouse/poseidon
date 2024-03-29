// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_STATIC_ASYNC_TASK_EXECUTOR_
#define POSEIDON_STATIC_ASYNC_TASK_EXECUTOR_

#include "../fwd.hpp"
namespace poseidon {

class Async_Task_Executor
  {
  private:
    mutable plain_mutex m_queue_mutex;
    condition_variable m_queue_avail;
    deque<wkptr<Abstract_Async_Task>> m_queue;

  public:
    // Creates an empty task executor.
    Async_Task_Executor() noexcept;

  public:
    Async_Task_Executor(const Async_Task_Executor&) = delete;
    Async_Task_Executor& operator=(const Async_Task_Executor&) & = delete;
    ~Async_Task_Executor();

    // Pops and executes a task.
    // This function should be called by the task thread repeatedly.
    void
    thread_loop();

    // Enqueues a task.
    // If this function fails, an exception is thrown, and there is no effect.
    // This function is thread-safe.
    void
    enqueue(shptrR<Abstract_Async_Task> task);
  };

}  // namespace poseidon
#endif
