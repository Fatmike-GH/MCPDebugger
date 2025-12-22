#include "WriteMemoryCommand.h"
#include "CommandResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>

namespace MCP::Native::Debugger::Commands
{
  void WriteMemoryCommand::Execute(Base::ICommandContext* context)
  {
    bool success = context->ExecuteWriteMemory(_address, _dataBuffer, _dataSize);
    SetResult(std::make_unique<CommandResult>(success));
  }
}