#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class SetBreakpointCommand : public Base::ICommand
  {
  public:
    SetBreakpointCommand(DWORD_PTR address)
      : ICommand(Base::CommandType::CMD_SET_BREAKPOINT),
      _address(address)
    {
    }

    void Execute(Base::ICommandContext* context) override;

  private:
    const DWORD_PTR _address;
  };
}