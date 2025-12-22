#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class StepOverCommand : public Base::ICommand
  {
  public:
    StepOverCommand()
      : ICommand(Base::CommandType::CMD_STEP_OVER)
    {
    }

    void Execute(Base::ICommandContext* context) override;
  };
}