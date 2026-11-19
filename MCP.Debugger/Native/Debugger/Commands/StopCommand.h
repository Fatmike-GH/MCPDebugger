#pragma once
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  /// <summary>
  /// Ends the debug session: tells its context that the session ended, which discards the commands still
  /// queued. It always succeeds.
  /// </summary>
  class StopCommand : public Base::ICommand
  {
  public:
    void Execute(Base::ICommandContext* context) override;
  };
}
