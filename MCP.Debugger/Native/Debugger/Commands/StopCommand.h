#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class StopCommand : public Base::ICommand
  {
  public:
    StopCommand()
      : ICommand(Base::CommandType::CMD_STOP)
    {
    }

    void Execute(Base::ICommandContext* context) override;
  };
}