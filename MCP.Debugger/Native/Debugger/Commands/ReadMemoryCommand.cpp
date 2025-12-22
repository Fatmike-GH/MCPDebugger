#include "ReadMemoryCommand.h"
#include "ReadMemoryResult.h"
#include ".\Base\ICommandContext.h"
#include <memory>
#include <vector>

namespace MCP::Native::Debugger::Commands
{
  void ReadMemoryCommand::Execute(Base::ICommandContext* context)
  {
    std::shared_ptr<std::vector<uint8_t>> data = nullptr;
    bool success = context->ExecuteReadMemory(_address, _dataSize, data);
    auto result = std::make_unique<Commands::ReadMemoryResult>(success, std::move(data));
    SetResult(std::move(result));
  }
}