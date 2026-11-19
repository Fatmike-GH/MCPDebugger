#pragma once
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  /// <summary>
  /// Lets the stopped debuggee run on. The command resumes nothing itself: it hands the plan of a continue
  /// to its context, and the debug thread carries the plan out. Its result tells whether the plan was
  /// accepted, which it is only at a stop.
  /// </summary>
  class ContinueCommand : public Base::ICommand
  {
  public:
    void Execute(Base::ICommandContext* context) override;
  };
}
