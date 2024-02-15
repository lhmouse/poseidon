// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_MONGO_QUERY_FUTURE_
#define POSEIDON_FIBER_MONGO_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_async_task.hpp"
#include "../mongo/mongo_value.hpp"
namespace poseidon {

class Mongo_Query_Future
  :
    public Abstract_Future,
    public Abstract_Async_Task
  {
  public:
    // This is actually an input/output type.
    struct Result
      {
        Mongo_Document cmd;  // input
        vector<Mongo_Document> reply_docs;
      };

  private:
    Mongo_Connector* m_connector;
    Result m_res;

  public:
    // Constructs a future for a single Mongo command. This object also functions
    // as an asynchronous task, which can be enqueued into an `Async_Task_Executor`.
    // This future will become ready once the command is complete.
    Mongo_Query_Future(Mongo_Connector& connector, Mongo_Document cmd);

  private:
    // Performs the BSON command.
    virtual
    void
    do_on_abstract_future_execute() override;

    virtual
    void
    do_on_abstract_async_task_execute() override;

  public:
    Mongo_Query_Future(const Mongo_Query_Future&) = delete;
    Mongo_Query_Future& operator=(const Mongo_Query_Future&) & = delete;
    virtual ~Mongo_Query_Future();

    bool
    has_result() const noexcept
      { return this->successful();  }

    const Result&
    result() const
      { return this->do_check_success(this->m_res);  }

    Result&
    mut_result()
      { return this->do_check_success(this->m_res);  }
  };

}  // namespace poseidon
#endif
