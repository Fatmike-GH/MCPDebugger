#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class RemoveBreakpointCommand : public Base::ICommand
  {
  public:
    RemoveBreakpointCommand(DWORD_PTR address)
      : ICommand(Base::CommandType::CMD_REMOVE_BREAKPOINT),
      _address(address)
    {
    }

    void Execute(Base::ICommandContext* context) override;

  private:
    const DWORD_PTR _address;
  };
}