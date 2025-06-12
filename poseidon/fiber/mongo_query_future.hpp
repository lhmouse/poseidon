// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_MONGO_QUERY_FUTURE_
#define POSEIDON_FIBER_MONGO_QUERY_FUTURE_

#include "../fwd.hpp"
#include "abstract_future.hpp"
#include "../base/abstract_task.hpp"
#include "../mongo/mongo_value.hpp"
namespace poseidon {

class Mongo_Query_Future
  :
    public Abstract_Future,
    public Abstract_Task
  {
  private:
    Mongo_Connector* m_ctr;
    uniptr<Mongo_Connection> m_conn;
    Mongo_Document m_cmd;
    cow_vector<Mongo_Document> m_res;

  public:
    // Constructs a future for a single Mongo command. This object also functions
    // as an asynchronous task, which can be enqueued into an `Task_Executor`.
    // This future will become ready once the command is complete.
    Mongo_Query_Future(Mongo_Connector& connector, uniptr<Mongo_Connection>&& conn_opt,
                       const Mongo_Document& cmd);

    Mongo_Query_Future(Mongo_Connector& connector, const Mongo_Document& cmd);

  private:
    virtual
    void
    do_on_abstract_future_initialize() override;

    virtual
    void
    do_on_abstract_future_finalize() override;

    virtual
    void
    do_on_abstract_task_execute() override;

  public:
    Mongo_Query_Future(const Mongo_Query_Future&) = delete;
    Mongo_Query_Future& operator=(const Mongo_Query_Future&) & = delete;
    virtual ~Mongo_Query_Future();

    // Gets the command to execute. This field is set by the constructor.
    const Mongo_Document&
    cmd() const noexcept
      { return this->m_cmd;  }

    // Gets the result after the operation has completed successfully. If
    // `successful()` yields `false`, an exception is thrown, and there is no
    // effect.
    const cow_vector<Mongo_Document>&
    result() const
      {
        this->check_success();
        return this->m_res;
      }
  };

}  // namespace poseidon
#endif
