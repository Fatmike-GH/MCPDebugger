#pragma once
#include "CommandResult.h"
#include ".\Base\ICommand.h"
#include ".\Base\CommandType.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  class Command : public Base::ICommand
  {
  public:
    Command(Base::CommandType type)
      : ICommand(type)
    {
    }

    void Execute(Base::ICommandContext* context) override
    {
      SetResult(std::make_unique<CommandResult>(true));
    }
  };
}

