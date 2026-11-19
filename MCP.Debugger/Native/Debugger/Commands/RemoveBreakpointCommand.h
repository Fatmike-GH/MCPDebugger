#pragma once
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class RemoveBreakpointCommand : public Base::ICommand
  {
  public:
    RemoveBreakpointCommand(DWORD_PTR address)
      : _address(address)
    {
    }

    void Execute(Base::ICommandContext* context) override;

  private:
    const DWORD_PTR _address;
  };
}