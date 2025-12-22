#pragma once
#include ".\Base\CommandType.h"
#include ".\Base\ICommand.h"

namespace MCP::Native::Debugger::Commands
{
  class WriteMemoryCommand : public Base::ICommand
  {
  public:
    WriteMemoryCommand(DWORD_PTR address, const BYTE* data, size_t size)
      : ICommand(Base::CommandType::CMD_WRITE_MEMORY),
      _address(address),
      _dataSize(size),
      _dataBuffer(data)
    {
    }

    void Execute(Base::ICommandContext* context) override;

  private:
    const DWORD_PTR _address;
    const size_t _dataSize;
    const BYTE* _dataBuffer;
  };
}