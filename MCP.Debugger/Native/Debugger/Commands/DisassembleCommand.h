#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class DisassembleCommand : public Base::ICommand
  {
  public:
    DisassembleCommand(DWORD_PTR address, size_t length)
      : ICommand(Base::CommandType::CMD_DISASSEMBLE),
        _address(address),
        _length(length)
    {
    }

    void Execute(Base::ICommandContext* context) override;

  private:
    const DWORD_PTR _address;
    const size_t _length;
  };
}