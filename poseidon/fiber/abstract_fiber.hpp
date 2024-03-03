// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_FIBER_ABSTRACT_FIBER_
#define POSEIDON_FIBER_ABSTRACT_FIBER_

#include "../fwd.hpp"
namespace poseidon {

class Abstract_Fiber
  {
  private:
    friend class Fiber_Scheduler;

  protected:
    // Constructs an inactive fiber.
    Abstract_Fiber() noexcept;

  protected:
    // This callback is invoked before `do_on_abstract_fiber_execute()`, and
    // after it is resumed from a previous yield operation.
    // This function should not throw exceptions; exceptions are ignored.
    // The default implementations merely print a message.
    virtual
    void
    do_on_abstract_fiber_resumed();

    // This callback is invoked by the fiber scheduler and is intended to be
    // overriden by derived classes to perform useful operation.
    virtual
    void
    do_on_abstract_fiber_execute() = 0;

    // This callback is invoked after `do_on_abstract_fiber_execute()`, and
    // before it is suspended by a yield operation.
    // This function should not throw exceptions; exceptions are ignored.
    // The default implementations merely print a message.
    virtual
    void
    do_on_abstract_fiber_suspended();

  public:
    Abstract_Fiber(const Abstract_Fiber&) = delete;
    Abstract_Fiber& operator=(const Abstract_Fiber&) & = delete;
    virtual ~Abstract_Fiber();
  };

}  // namespace poseidon
#endif
