#pragma once
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  /// <summary>
  /// Lets the stopped debuggee execute one instruction, running past a call instead of into it. The
  /// command resumes nothing itself: it hands the plan of a step over to its context, and the debug thread
  /// carries the plan out. Its result tells whether the plan was accepted, which it is only at a stop.
  /// </summary>
  class StepOverCommand : public Base::ICommand
  {
  public:
    void Execute(Base::ICommandContext* context) override;
  };
}
