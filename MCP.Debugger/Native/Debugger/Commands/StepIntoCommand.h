#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class StepIntoCommand : public Base::ICommand
  {
  public:
    StepIntoCommand()
      : ICommand(Base::CommandType::CMD_STEP_INTO)
    {
    }

    void Execute(Base::ICommandContext* context) override;
  };
}