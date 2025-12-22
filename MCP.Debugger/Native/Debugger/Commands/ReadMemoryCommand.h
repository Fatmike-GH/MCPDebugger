#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class ReadMemoryCommand : public Base::ICommand
  {
  public:
    ReadMemoryCommand(DWORD_PTR address, size_t size)
      : ICommand(Base::CommandType::CMD_READ_MEMORY),
        _address(address),
        _dataSize(size)
    {
    }

    void Execute(Base::ICommandContext* context) override;

  private:
    const DWORD_PTR _address;
    const size_t _dataSize;
  };
}