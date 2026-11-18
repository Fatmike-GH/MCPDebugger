#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  /// <summary>
  /// Lets the stopped debuggee run on. The command itself resumes nothing: the debug thread acts on its type
  /// when the command ends the processing of a stop. Its result tells whether it was executed at a stop - only
  /// then it takes effect.
  /// </summary>
  class ContinueCommand : public Base::ICommand
  {
  public:
    ContinueCommand()
      : ICommand(Base::CommandType::CMD_CONTINUE)
    {
    }

    void Execute(Base::ICommandContext* context) override;
  };
}
