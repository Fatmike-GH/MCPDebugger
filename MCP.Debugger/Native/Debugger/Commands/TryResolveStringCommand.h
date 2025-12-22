#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class TryResolveStringCommand : public Base::ICommand
  {
  public:
    TryResolveStringCommand(DWORD_PTR address)
      : ICommand(Base::CommandType::CMD_TRY_RESOLVE_STRING),
        _address(address)
    {
    }

    void Execute(Base::ICommandContext* context) override;

  private:
    const DWORD_PTR _address;
  };
}